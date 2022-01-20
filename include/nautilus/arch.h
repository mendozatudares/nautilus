#pragma once

#include <nautilus/nautilus.h>

/* arch specific */
void arch_enable_ints(void);
void arch_disable_ints(void);
int  arch_ints_enabled(void);

int  arch_early_init (struct naut_info * naut);
int  arch_numa_init(struct sys_info* sys);
void arch_paging_init(struct nk_mem_info * mem, ulong_t mbd);

void arch_detect_mem_map (mmap_info_t * mm_info, mem_map_entry_t * memory_map, unsigned long mbd);
void arch_reserve_boot_regions(unsigned long mbd);

uint32_t arch_cycles_to_ticks(uint64_t cycles);
uint32_t arch_realtime_to_ticks(uint64_t ns);
uint64_t arch_realtime_to_cycles(uint64_t ns);
uint64_t arch_cycles_to_realtime(uint64_t cycles);

void arch_update_timer(uint32_t ticks, nk_timer_condition_t cond);
void arch_set_timer(uint32_t ticks);
int  arch_read_timer(void);
int  arch_timer_handler(excp_entry_t * excp, excp_vec_t vec, void *state);
uint64_t arch_read_timestamp(void);

#ifdef NAUT_CONFIG_ARCH_X86
#include <arch/x64/arch.h>
#else
#ifdef NAUT_CONFIG_ARCH_RISCV
#include <arch/riscv/arch.h>
#else
#error "Unsupported architecture"
#endif
#endif
