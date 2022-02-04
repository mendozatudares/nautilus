#include <arch/riscv/plic.h>
#include <arch/riscv/sbi.h>
#include <nautilus/cpu.h>
#include <nautilus/devicetree.h>
#include <nautilus/naut_types.h>
#include <nautilus/percpu.h>

static addr_t plic_addr = 0;

#define MREG(x) *((uint32_t *)x)
#define PLIC           plic_addr
#define PLIC_PRIORITY MREG(PLIC + 0x0)
#define PLIC_PENDING MREG(PLIC + 0x1000)
#define PLIC_MENABLE(hart) MREG(PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) MREG(PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) MREG(PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) MREG(PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) MREG(PLI + 0x201004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) MREG(PLIC + 0x200004 + (hart)*0x2000)

#define ENABLE_BASE 0x2000
#define ENABLE_PER_HART 0x100

bool_t dtb_node_plic_compatible(struct dtb_node *n) {
    for (off_t i = 0; i < n->ncompat; i++) {
        if (strstr(n->compatible[i], "plic")) {
            return true;
        }
    }
    return false;
}

bool_t dtb_node_get_plic(struct dtb_node *n) {
    if (strstr(n->name, "interrupt-controller") && dtb_node_plic_compatible(n)) {
        PLIC = n->address;
        return false;
    }
    return true;
}

void plic_init(void) {
    dtb_walk_devices(dtb_node_get_plic);
}

static void plic_toggle(int hart, int hwirq, int priority, bool_t enable) {
    printk("toggling on hart %d, irq=%d, priority=%d, enable=%d\n", hart, hwirq, priority, enable);
    off_t enable_base = PLIC + ENABLE_BASE + hart * ENABLE_PER_HART;
    uint32_t* reg = &(MREG(enable_base + (hwirq / 32) * 4));
    uint32_t hwirq_mask = 1 << (hwirq % 32);
    MREG(PLIC + 4 * hwirq) = 7;
    PLIC_SPRIORITY(hart) = 0;

    if (enable) {
    *reg = *reg | hwirq_mask;
    } else {
    *reg = *reg & ~hwirq_mask;
    }
}

void plic_enable(int hwirq, int priority)
{
    plic_toggle(my_cpu_id(), hwirq, priority, true);
}
void plic_disable(int hwirq)
{
    plic_toggle(my_cpu_id(), hwirq, 0, false);
}
int plic_claim(void)
{
    return PLIC_SCLAIM(my_cpu_id());
}
void plic_complete(int irq)
{
    PLIC_SCLAIM(my_cpu_id()) = irq;
}
int plic_pending(void)
{
    return PLIC_PENDING;
}


/* static int boot_hart = -1; */

void plic_init_hart(void) {
    int hart = my_cpu_id();

    for (int i = 0; i < 0x1000 / 4; i++)
        MREG(PLIC + i * 4) = 7;

    (&PLIC_SENABLE(hart))[0] = 0;
    (&PLIC_SENABLE(hart))[1] = 0;
    (&PLIC_SENABLE(hart))[2] = 0;
}

