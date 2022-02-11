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
