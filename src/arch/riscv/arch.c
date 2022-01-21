#include <nautilus/arch.h>

void arch_enable_ints(void)  { set_csr(sstatus, SSTATUS_SIE); }
void arch_disable_ints(void) { clear_csr(sstatus, SSTATUS_SIE); }
int  arch_ints_enabled(void) { return read_csr(sstatus) & SSTATUS_SIE; };

uint32_t arch_cycles_to_ticks(uint64_t cycles) { /* TODO */ return cycles; }
uint32_t arch_realtime_to_ticks(uint64_t ns) { return ((ns*RISCV_CLOCKS_PER_SECOND)/1000000000ULL); }
uint64_t arch_realtime_to_cycles(uint64_t ns) { /* TODO */ return arch_realtime_to_ticks(ns); }
uint64_t arch_cycles_to_realtime(uint64_t cycles) { return (cycles * 1000000000ULL)/RISCV_CLOCKS_PER_SECOND; }

#include <arch/riscv/sbi.h>
#include <nautilus/scheduler.h>
#include <nautilus/timer.h>

static uint8_t timer_set = 0;
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
        if (ticks < current_ticks) { arch_set_timer(ticks);}
        break;
    case IF_LATER:
        if (ticks > current_ticks) { arch_set_timer(ticks);}
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

    if (time_to_next_ns == 0) {
	// indicates "infinite", which we turn into the maximum timer count
	arch_set_timer(-1);
    } else {
	arch_set_timer(arch_realtime_to_ticks(time_to_next_ns));
    }

    nk_yield();

    return 0;
}

uint64_t arch_read_timestamp() { return read_csr(time); }
