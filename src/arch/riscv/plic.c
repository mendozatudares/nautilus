#include <arch/riscv/plic.h>
#include <arch/riscv/sbi.h>
#include <nautilus/cpu.h>
#include <nautilus/devicetree.h>
#include <nautilus/naut_types.h>
#include <nautilus/percpu.h>

addr_t plic_addr = 0;
#define PLIC plic_addr
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

bool dtb_node_get_plic(struct dtb_node *n) {
	// printk("%s %s\n", n->name, n->compatible);
    if (strstr(n->name, "interrupt-controller") && strstr(n->compatible, "plic")) {
        // printk("This one looks good.\n");
        PLIC = n->address;
        return false;
    }
    return true;
}

void plic_init(void) {
    dtb_walk_devices(dtb_node_get_plic);
    // printk("PLIC @ %p\n", plic_addr);

    if (plic_addr != 0) {
        // set desired IRQ priorities non-zero (otherwise disabled).
        *(uint32_t *)(PLIC + UART0_IRQ * 4) = 1;
        *(uint32_t *)(PLIC + VIRTIO0_IRQ * 4) = 1;
    }
}

int boot_hart = -1;

void plic_init_hart(void) {
    int hart = my_cpu_id();

    // Clear the "supervisor enable" field. This is the register that enables or
    // disables external interrupts (UART, DISK, ETC)
    *(uint32_t *)PLIC_SENABLE(hart) = 0;

    // set this hart's S-mode priority threshold to 0.
    *(uint32_t *)PLIC_SPRIORITY(hart) = 0;

    if (boot_hart == -1) {
        boot_hart = hart;
    } else {
        *(uint32_t *)PLIC_SENABLE(hart) = PLIC_SENABLE(boot_hart);
    }
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void) { return PLIC_SCLAIM(my_cpu_id()); }

// tell the PLIC we've served this IRQ.
void plic_complete(int irq) { *(uint32_t *)PLIC_SCLAIM(my_cpu_id()) = irq; }

void plic_enable(int irq, int priority) {
    int hart = my_cpu_id();
    *(uint32_t *)(PLIC + irq * 4) = 1;
    *(uint32_t *)PLIC_SENABLE(hart) |= (1 << irq);
    *(uint32_t *)PLIC_SPRIORITY(hart) |= (1 << irq);
}

void plic_disable(int irq) {
    *(uint32_t *)PLIC_SENABLE(my_cpu_id()) &= ~(1 << irq);
}

/* Some fakery to get the scheduler working */

#include <nautilus/timer.h>

static uint8_t timer_set = 0;
static uint32_t current_ticks = 0;
int in_timer_interrupt = 0;
int in_kick_interrupt = 0;
static uint64_t timer_count = 0;

uint32_t apic_realtime_to_ticks(struct apic_dev *apic, uint64_t ns)
{

    return ((ns*1000000000ULL)/RISCV_CLOCKS_PER_SECOND);
}

uint64_t apic_cycles_to_realtime(struct apic_dev *apic, uint64_t cycles)
{
    return 1000000000ULL*(cycles/RISCV_CLOCKS_PER_SECOND);
}

void apic_set_oneshot_timer(struct apic_dev *apic, uint32_t ticks) 
{
    if (!ticks) {
    sbi_set_timer(r_time() + 1);
    } else if (ticks == -1 && current_ticks != -1) {
    sbi_set_timer(-1);
    } else {
    sbi_set_timer(r_time() + ticks);
    }
    timer_set = 1;
    current_ticks = ticks;
}

void apic_update_oneshot_timer(struct apic_dev *apic, uint32_t ticks,
			       nk_timer_condition_t cond) {
    if (!timer_set) {
    apic_set_oneshot_timer(apic, ticks);
    } else {
    switch(cond) {
    case UNCOND:
        apic_set_oneshot_timer(apic, ticks);
        break;
    case IF_EARLIER:
        if (ticks < current_ticks) { apic_set_oneshot_timer(apic,ticks);}
        break;
    case IF_LATER:
        if (ticks > current_ticks) { apic_set_oneshot_timer(apic,ticks);}
        break;
    }
    }
    // note that this is set at the entry to apic_timer_handler
    in_timer_interrupt=0;
    // note that this is set at the entry to null_kick
    in_kick_interrupt=0;
}

static int apic_timer_handler(excp_entry_t * excp, excp_vec_t vec, void *state)
{
    struct apic_dev * apic = (struct apic_dev*)per_cpu_get(apic);

    uint64_t time_to_next_ns;

    in_timer_interrupt=1;

    timer_count++;

    timer_set = 0;

    // do all our callbacks
    // note that currently all cores see the events
    time_to_next_ns = nk_timer_handler();

    // note that the low-level interrupt handler code in excp_early.S
    // takes care of invoking the scheduler if needed, and the scheduler
    // will in turn change the time after we leave - it may set the
    // timer to expire earlier.  The scheduler can refer to 
    // the apic to determine that it is being invoked in timer interrupt
    // context.   In this way, it can differentiate:
    //   1. invocation from a thread
    //   2. invocation from a interrupt context for some other interrupt
    //   3. invocation from a timer interrupt context
    // This is important as a flaky APIC timer (we're looking at you, QEMU)
    // can fire before the expected elapsed time expires.  If the scheduler
    // doesn't reset the timer when this happens, the scheduler may be delayed
    // as far as the next interrupt or cooperative rescheduling request,
    // breaking real-time semantics.  

    if (time_to_next_ns == -1) { 
	// indicates "infinite", which we turn into the maximum timer count
	apic_set_oneshot_timer(apic,-1);
    } else {
	apic_set_oneshot_timer(apic,apic_realtime_to_ticks(apic,time_to_next_ns));
    }

    return 0;
}

void plic_timer_handler(void)
{
    apic_timer_handler(0,0,0);
}

