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
#include <nautilus/devicetree.h>

#ifdef NAUT_CONFIG_ENABLE_REMOTE_DEBUGGING
#include <nautilus/gdb-stub.h>
#endif

#include <arch/riscv/riscv.h>
#include <arch/riscv/sbi.h>
#include <arch/riscv/plic.h>
#include <arch/riscv/memlayout.h>
#include <arch/riscv/trap.h>


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

extern uint64_t secondary_core_startup_sbi;
extern uint64_t secondary_core_stack;
extern int uart_getchar(void);

bool second_done = false;

void secondary_entry(int hartid) {
    
    struct naut_info * naut = &nautilus_info;

    w_sscratch(r_tp());

    w_tp((uint64_t)naut->sys.cpus[hartid]);

    /* Initialize the platform level interrupt controller for this HART */
    plic_init_hart();

    // Write supervisor trap vector location
    trap_init_hart();

    /* set the timer with sbi :) */
    // sbi_set_timer(rv::get_time() + TICK_INTERVAL);

    second_done = true;

    sti();

    while (1) {
    }
}

int start_secondary(void) {
    for (int i = 0; i < NAUT_CONFIG_MAX_CPUS; i++) {
        if (i == my_cpu_id()) continue;

        printk("RISCV: Hart %d trying to start hart %d\n", my_cpu_id(), i);

        secondary_core_stack = (uint64_t)malloc(2 * 4096);
        secondary_core_stack += 2 * 4096;

        struct sbiret ret = sbi_call(SBI_EXT_HSM, SBI_EXT_HSM_HART_START, i, &secondary_core_startup_sbi, 0);
        if (ret.error != SBI_SUCCESS) continue;

        while (second_done != true) {
        //     // __sync_synchronize();
        }
        printk("RISCV: Hart %d successfully started hart %d\n", my_cpu_id(), i);
    }

    return 0;
}

void init (int hartid, void* fdt) {

	long x = 0;
	printk("x: %llx, fdt: %llx\n", x, fdt);

    if (!fdt) panic("reboot\n");

    // Get necessary information from SBI
    sbi_early_init();

    // M-Mode passes scratch struct through tp. Move it to sscratch
    w_sscratch(r_tp());

    // Zero out tp for now until tls is set up
    w_tp(0);

    struct naut_info * naut = &nautilus_info;
    nk_low_level_memset(naut, 0, sizeof(struct naut_info));

    naut->sys.bsp_id = hartid;
    naut->sys.dtb = fdt;

	
    dtb_parse((struct dtb_fdt_header *) fdt);

    printk(NAUT_WELCOME);

    // Setup the temporary boot-time allocator
    mm_boot_init((ulong_t) fdt);

    // Enumate CPUs and initialize them
    smp_early_init(naut);

    /* this will populate NUMA-related structures */
    arch_numa_init(&naut->sys);

    // Setup the main kernel memory allocator
    nk_kmem_init();

    // Setup per-core area for BSP
    w_tp((uint64_t)naut->sys.cpus[hartid]);

    /* now we switch to the real kernel memory allocator, pages
     * allocated in the boot mem allocator are kept reserved */
    mm_boot_kmem_init();

    // Write supervisor trap vector location
    trap_init_hart();

    // Initialize platform level interrupt controller for this HART
    plic_init();

    plic_init_hart();

    /* from this point on, we can use percpu macros (even if the APs aren't up) */

    sysinfo_init(&(naut->sys));

    // nk_wait_queue_init();

    // nk_sched_init(&sched_cfg);

    mm_boot_kmem_cleanup();

    sti();

    sbi_init();

    /* interrupts are now on */

    // start_secondary();

    while(1) {
        int c = uart_getchar();
        if (c != -1) {
            if (c == 13) printk("\n");
            else printk("%c", c);
        }
    }
}
