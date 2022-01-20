#include <nautilus/arch.h>

void arch_enable_ints(void)  { set_csr(sstatus, SSTATUS_SIE); }
void arch_disable_ints(void) { clear_csr(sstatus, SSTATUS_SIE); }
int  arch_ints_enabled(void) { return read_csr(sstatus) & SSTATUS_SIE; };

int  arch_early_init (struct naut_info * naut) { return riscv_early_init(naut); }
int  arch_numa_init(struct sys_info* sys) { return riscv_numa_init(sys); }
void arch_paging_init(struct nk_mem_info * mem, ulong_t mbd) { riscv_paging_init(mem, mbd); }

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

#include <arch/riscv/sbi.h>
#include <nautilus/scheduler.h>
#include <nautilus/timer.h>

int in_timer_interrupt = 0;
int in_kick_interrupt = 0;
static uint8_t timer_set = 0;
static uint32_t current_ticks = 0;
static uint64_t timer_count = 0;

void arch_update_timer(uint32_t ticks, nk_timer_condition_t cond) {
    apic_update_oneshot_timer(MY_APIC, ticks, cond);
}
void arch_set_timer(uint32_t ticks) { apic_set_oneshot_timer(MY_APIC, ticks); }
int  arch_read_timer(void) { return apic_read_timer(MY_APIC); }
int  arch_timer_handler(excp_entry_t * excp, excp_vec_t vec, void *state) {
    return apic_timer_handler(MY_APIC, excp, vec, state);
}
uint64_t arch_read_timestamp() { return rtdsc(); }
