/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the
 * United States National  Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kyle C. Hale <kh@u.northwestern.edu>
 *          Peter Dinda <pdinda@northwestern.edu> (Gem5 variant)
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#define __NAUTILUS_MAIN__

#include <nautilus/nautilus.h>
#include <nautilus/spinlock.h>
#include <nautilus/mb_utils.h>
#include <nautilus/cpu.h>
#include <nautilus/percpu.h>
#include <nautilus/errno.h>

#ifdef NAUT_CONFIG_ENABLE_REMOTE_DEBUGGING
#include <nautilus/gdb-stub.h>
#endif

#include "init.h"
#include "riscv.h"

uint64_t kernel_page_table[4096 / sizeof(uint64_t)] __attribute__((aligned(4096)));

#define QUANTUM_IN_NS (1000000000ULL/NAUT_CONFIG_HZ)

struct nk_sched_config sched_cfg = {
    .util_limit = NAUT_CONFIG_UTILIZATION_LIMIT*10000ULL, // convert percent to 10^-6 units
    .sporadic_reservation =  NAUT_CONFIG_SPORADIC_RESERVATION*10000ULL, // ..
    .aperiodic_reservation = NAUT_CONFIG_APERIODIC_RESERVATION*10000ULL, // ..
    .aperiodic_quantum = QUANTUM_IN_NS,
    .aperiodic_default_priority = QUANTUM_IN_NS,
};

static int
sysinfo_init (struct sys_info * sys)
{
    sys->core_barrier = (nk_barrier_t*)malloc(sizeof(nk_barrier_t));
    if (!sys->core_barrier) {
        ERROR_PRINT("Could not allocate core barrier\n");
        return -1;
    }
    memset(sys->core_barrier, 0, sizeof(nk_barrier_t));

    if (nk_barrier_init(sys->core_barrier, sys->num_cpus) != 0) {
        ERROR_PRINT("Could not create core barrier\n");
        goto out_err;
    }

    return 0;

out_err:
    free(sys->core_barrier);
    return -EINVAL;
}

uint32_t
nk_get_num_cpus (void)
{
    struct sys_info * sys = per_cpu_get(system);
    return sys->num_cpus;
}

#define NAUT_WELCOME \
"Welcome to                                         \n" \
"    _   __               __   _  __                \n" \
"   / | / /____ _ __  __ / /_ (_)/ /__  __ _____    \n" \
"  /  |/ // __ `// / / // __// // // / / // ___/    \n" \
" / /|  // /_/ // /_/ // /_ / // // /_/ /(__  )     \n" \
"/_/ |_/ \\__,_/ \\__,_/ \\__//_//_/ \\__,_//____/  \n" \
"+===============================================+  \n" \
" Kyle C. Hale (c) 2014 | Northwestern University   \n" \
"+===============================================+  \n\n"

static struct multiboot_info riscv_fake_multiboot_info = {
  .boot_loader = "RISCV with fakery",
  .boot_cmd_line = "nautilus.bin",
  .sec_hdr_start = 0,
  .hrt_info = 0
};


void
init (unsigned long mbd,
      unsigned long magic)
{
    struct naut_info * naut = &nautilus_info;

    nk_low_level_memset(naut, 0, sizeof(struct naut_info));

    // Initialize interrupts
    // nk_int_init(&(naut->sys));
    trap_init_hart();
    plic_init();
    plic_init_hart();

    // Bring up UART device and printing so we can have output
    uart_init();

    printk_init();

    printk(NAUT_WELCOME);

    /* setup the temporary boot-time allocator */
    /* this will detect memory from an array in memory, not multiboot or e820 */
    mm_boot_init(mbd);

    naut->sys.mb_info = &riscv_fake_multiboot_info;

    /* initialize boot CPU */
    arch_early_init(naut);

    /* this will finish up the identity map */
    // nk_paging_init(&(naut->sys.mem), mbd);

    /* setup the main kernel memory allocator */
    // nk_kmem_init();

    /* setup per-core area for BSP */
    w_tp((uint64_t)naut->sys.cpus[0]);

    sti();

    while(1);
}
