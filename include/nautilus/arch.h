#pragma once

#include <nautilus/mm.h>


// arch_enable_ints
// arch_disable_ints
// arch_ints_enabled

// arch_early_init
// arch_numa_init
// arch_paging_init


/* arch specific */
void arch_detect_mem_map (mmap_info_t * mm_info, mem_map_entry_t * memory_map, unsigned long mbd);
void arch_reserve_boot_regions(unsigned long mbd);

// arch_cycles_to_ticks
// arch_realtime_to_ticks
// arch_realtime_to_cycles
// arch_cycles_to_realtime
// arch_set_timer
// arch_update_timer
// arch_read_timer
