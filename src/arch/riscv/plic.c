#include <arch/riscv/plic.h>
#include <arch/riscv/sbi.h>
#include <nautilus/cpu.h>
#include <nautilus/devicetree.h>
#include <nautilus/naut_types.h>
#include <nautilus/percpu.h>

static addr_t plic_addr = 0;

#define MREG(x) *((uint32_t *)x)

#define PLIC plic_addr
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
        printk("PLIC @ %p\n", n->address);
        PLIC = n->address;
        return false;
    }
    return true;
}

void plic_init(void) {
    /* dtb_walk_devices(dtb_node_get_plic); */
    PLIC = 0x0c000000L;
}

static void plic_toggle(int hart, int hwirq, int priority, bool_t enable) {
    printk("toggling on hart %d, irq=%d, priority=%d, enable=%d, plic=%p\n", hart, hwirq, priority, enable, PLIC);
    off_t enable_base = PLIC + ENABLE_BASE + hart * ENABLE_PER_HART;
    printk("enable_base=%p\n", enable_base);
    uint32_t* reg = &(MREG(enable_base + (hwirq / 32) * 4));
    printk("reg=%p\n", reg);
    uint32_t hwirq_mask = 1 << (hwirq % 32);
    printk("hwirq_mask=%p\n", hwirq_mask);
    MREG(PLIC + 4 * hwirq) = 7;
    PLIC_SPRIORITY(hart) = 0;

    if (enable) {
    *reg = *reg | hwirq_mask;
    printk("*reg=%p\n", *reg);
    } else {
    *reg = *reg & ~hwirq_mask;
    }

    printk("*reg=%p\n", *reg);
}

void plic_enable(int hwirq, int priority)
{
    plic_toggle(1, hwirq, priority, true);
}
void plic_disable(int hwirq)
{
    plic_toggle(1, hwirq, 0, false);
}
int plic_claim(void)
{
    return PLIC_SCLAIM(1);
}
void plic_complete(int irq)
{
    PLIC_SCLAIM(1) = irq;
}
int plic_pending(void)
{
    return PLIC_PENDING;
}

void plic_dump(void)
{
    printk("Dumping SiFive PLIC Register Map\n");

    printk("source priorities:\n");
    uint32_t* addr = 0x0c000000L;
    for (off_t i = 1; i < 54; i++) {
        printk("  %p (source %2d) = %d\n", addr + i, i, MREG(addr + i));
    }

    addr = (uint32_t*)0x0c001000L;
    printk("pending array:\t\t\t\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c002000L;
    printk("hart 0 m-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c002080L;
    printk("hart 1 m-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));
    addr = (uint32_t*)0x0c002100L;
    printk("hart 1 s-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));


    addr = (uint32_t*)0x0c002180L;
    printk("hart 2 m-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));
    addr = (uint32_t*)0x0c002200L;
    printk("hart 2 s-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c002280L;
    printk("hart 3 m-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));
    addr = (uint32_t*)0x0c002300L;
    printk("hart 3 s-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));


    addr = (uint32_t*)0x0c002380L;
    printk("hart 4 m-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));
    addr = (uint32_t*)0x0c002400L;
    printk("hart 4 s-mode interrupt enables:\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c200000L;
    printk("hart 0 m-mode priority threshold:\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c200004L;
    printk("hart 0 m-mode claim/complete:\t\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c201000L;
    printk("hart 1 m-mode priority threshold:\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c201004L;
    printk("hart 1 m-mode claim/complete\t\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c202000L;
    printk("hart 1 s-mode priority threshold:\t%p = %p\n", addr, MREG(addr));

    addr = (uint32_t*)0x0c202004L;
    printk("hart 1 s-mode claim/complete:\t\t%p = %p\n", addr, MREG(addr));



}

void plic_init_hart(int hart) {
    PLIC_SPRIORITY(hart) = 0;
}

