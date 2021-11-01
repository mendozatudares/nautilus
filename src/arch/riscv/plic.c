#include <nautilus/naut_types.h>
#include <nautilus/cpu.h>
#include <nautilus/percpu.h>
#include <nautilus/devicetree.h>
#include <arch/riscv/sbi.h>
#include <arch/riscv/plic.h>

addr_t plic_addr;
#define PLIC plic_addr
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

bool dtb_node_get_plic (struct dtb_node * n) {
    if(!strcmp(n->name, "interrupt-controller") && strstr(n->compatible, "plic0")) {
        PLIC = n->reg.address;
        return false;
    }
    return true;
}

void
plic_init(void)
{
    dtb_walk_devices(dtb_node_get_plic);

    // set desired IRQ priorities non-zero (otherwise disabled).
    *(uint32_t*)(PLIC + UART0_IRQ*4) = 1;
    *(uint32_t*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

int boot_hart = -1;

void
plic_init_hart(void)
{
    int hart = my_cpu_id();

    // Clear the "supervisor enable" field. This is the register that enables or disables external interrupts (UART, DISK, ETC)
    *(uint32_t*)PLIC_SENABLE(hart) = 0;

    // set this hart's S-mode priority threshold to 0.
    *(uint32_t*)PLIC_SPRIORITY(hart) = 0;

    if (boot_hart == -1) {
        boot_hart = hart;
    } else {
        *(uint32_t*)PLIC_SENABLE(hart) = PLIC_SENABLE(boot_hart);
    } 
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
    return PLIC_SCLAIM(my_cpu_id());
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
    *(uint32_t*)PLIC_SCLAIM(my_cpu_id()) = irq;
}

void
plic_enable(int irq, int priority) {
    int hart = my_cpu_id();
    *(uint32_t*)(PLIC + irq * 4) = 1;
    *(uint32_t*)PLIC_SENABLE(hart) |= (1 << irq);
    *(uint32_t*)PLIC_SPRIORITY(hart) |= (1 << irq);
}

void
plic_disable(int irq) {
    *(uint32_t*)PLIC_SENABLE(my_cpu_id()) &= ~(1 << irq);
}

/* Some fakery to get the scheduler working */
uint64_t apic_cycles_to_realtime(struct apic_dev *apic, uint64_t cycles)
{
    return 1000ULL*(cycles/1);
    // return 1000ULL*(cycles/(apic->cycles_per_us));
}

uint32_t apic_realtime_to_ticks(struct apic_dev *apic, uint64_t ns)
{
    return ((ns*1000ULL)/1);
    // return ((ns*1000ULL)/apic->ps_per_tick);
}

void apic_update_oneshot_timer(struct apic_dev *apic, uint32_t ticks,
			       nk_timer_condition_t cond)
{
    return;
}
