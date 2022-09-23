#include <nautilus/arch.h>

void arch_enable_ints(void)  { set_csr(sstatus, SSTATUS_SIE); }
void arch_disable_ints(void) { clear_csr(sstatus, SSTATUS_SIE); }
int  arch_ints_enabled(void) { return read_csr(sstatus) & SSTATUS_SIE; };

#include <arch/riscv/plic.h>

void arch_irq_enable(int irq) { plic_enable(irq, 1); }
void arch_irq_disable(int irq) { 
    printk("im disabling irq=%d\n", irq);
    plic_disable(irq); }
void arch_irq_install(int irq, int (*handler)(excp_entry_t *, excp_vec_t, void *)) {
    printk("registering int handler! irq=%d, handler=%p\n", irq, handler);
    register_int_handler(irq, handler);
    arch_irq_enable(irq);
}
void arch_irq_uninstall(int irq) { /* TODO */ }

uint32_t arch_cycles_to_ticks(uint64_t cycles) { /* TODO */ return cycles; }
uint32_t arch_realtime_to_ticks(uint64_t ns) { return ((ns*RISCV_CLOCKS_PER_SECOND)/1000000000ULL); }
uint64_t arch_realtime_to_cycles(uint64_t ns) { /* TODO */ return arch_realtime_to_ticks(ns); }
uint64_t arch_cycles_to_realtime(uint64_t cycles) { return (cycles * 1000000000ULL)/RISCV_CLOCKS_PER_SECOND; }

#include <arch/riscv/sbi.h>
#include <nautilus/scheduler.h>
#include <nautilus/timer.h>

static uint8_t  timer_set = 0;
static uint32_t current_ticks = 0;
static uint64_t timer_count = 0;

void arch_update_timer(uint32_t ticks, nk_timer_condition_t cond) {
    if (!timer_set) {
    arch_set_timer(ticks);
    } else {
    switch(cond) {
    case UNCOND:
        arch_set_timer(ticks);
        break;
    case IF_EARLIER:
        if (ticks < current_ticks) { arch_set_timer(ticks); }
        break;
    case IF_LATER:
        if (ticks > current_ticks) { arch_set_timer(ticks); }
        break;
    }
    }
    get_cpu()->in_timer_interrupt=0;
    get_cpu()->in_kick_interrupt=0;
}

void arch_set_timer(uint32_t ticks) {
    uint64_t time_to_set = !ticks ? read_csr(time) + 1 :
           (ticks != -1 || current_ticks == -1) ? read_csr(time) + ticks : -1;
    sbi_set_timer(time_to_set);
    timer_set = 1;
    current_ticks = ticks;
}

int  arch_read_timer(void) { /* TODO */ return 0; }

int  arch_timer_handler(excp_entry_t * excp, excp_vec_t vec, void *state)
{
    uint64_t time_to_next_ns;

    get_cpu()->in_timer_interrupt=1;

    timer_count++;

    timer_set = 0;

    time_to_next_ns = nk_timer_handler();

    if (time_to_next_ns == 0) {
    arch_set_timer(-1);
    } else {
    arch_set_timer(arch_realtime_to_ticks(time_to_next_ns));
    }

    nk_yield();

    return 0;
}

uint64_t arch_read_timestamp() { return read_csr(time); }

void arch_print_regs(struct nk_regs * r) {

    printk("RA:  %016lx SP:  %016lx\n", r->ra, r->sp);
    printk("GP:  %016lx TP:  %016lx\n", r->gp, r->tp);
    printk("T00: %016lx T01: %016lx T02: %016lx\n", r->t0, r->t1, r->t2);
    printk("S00: %016lx S01: %016lx A00: %016lx\n", r->s0, r->s1, r->a0);
    printk("A01: %016lx A02: %016lx A03: %016lx\n", r->a1, r->a2, r->a3);
    printk("A04: %016lx A05: %016lx A06: %016lx\n", r->a4, r->a5, r->a6);
    printk("A07: %016lx S02: %016lx S03: %016lx\n", r->a7, r->s2, r->s3);
    printk("S04: %016lx S05: %016lx S06: %016lx\n", r->s4, r->s5, r->s6);
    printk("S07: %016lx S08: %016lx S09: %016lx\n", r->s7, r->s8, r->s9);
    printk("S10: %016lx S11: %016lx T03: %016lx\n", r->s10, r->s11, r->t3);
    printk("T04: %016lx T05: %016lx T06: %016lx\n", r->t4, r->t5, r->t6);

}

void * arch_read_sp(void) {
    void * sp = NULL;
    __asm__ __volatile__ ( "mv %[_r], sp" : [_r] "=r" (rsp) : : "memory" );
    return sp;
}

void arch_relax(void) {
    asm volatile ("pause");
}

void arch_halt(void) {
    asm volatile ("hlt");
}

