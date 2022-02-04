#include <nautilus/arch.h>

void arch_enable_ints(void)  { asm volatile ("sti" : : : "memory"); }
void arch_disable_ints(void) { asm volatile ("cli" : : : "memory"); }
int  arch_ints_enabled(void) {
    uint64_t rflags = 0;
    asm volatile("pushfq; popq %0" : "=a"(rflags));
    return (rflags & RFLAGS_IF) != 0;
}

#include <dev/apic.h>
#define MY_APIC per_cpu_get(system)->cpus[my_cpu_id()]->apic

uint32_t arch_cycles_to_ticks(uint64_t cycles) {
    return apic_cycles_to_ticks(MY_APIC, cycles);
}
uint32_t arch_realtime_to_ticks(uint64_t ns) {
    return apic_realtime_to_ticks(MY_APIC, ns);
}
uint64_t arch_realtime_to_cycles(uint64_t ns) {
    return apic_realtime_to_cycles(MY_APIC, ns);
}
uint64_t arch_cycles_to_realtime(uint64_t cycles) {
    return apic_cycles_to_realtime(MY_APIC, cycles);
}

#include <nautilus/scheduler.h>

void arch_update_timer(uint32_t ticks, nk_timer_condition_t cond) {
    apic_update_oneshot_timer(MY_APIC, ticks, cond);
}
void arch_set_timer(uint32_t ticks) { apic_set_oneshot_timer(MY_APIC, ticks); }
int  arch_read_timer(void) { return apic_read_timer(MY_APIC); }
int  arch_timer_handler(excp_entry_t * excp, excp_vec_t vec, void *state) {
    return apic_timer_handler(excp, vec, state);
}
uint64_t arch_read_timestamp() { return rdtsc(); }
