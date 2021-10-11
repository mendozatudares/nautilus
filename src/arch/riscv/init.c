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

void trap_init(void);
void uart_init(void);
int uart_getchar(void);

void init (int hartid, void* fdt) {

    // Get necessary information from SBI
    sbi_early_init();

    // M-Mode passes scratch struct through tp. Move it to sscratch
    w_sscratch(r_tp());

    struct naut_info * naut = &nautilus_info;
    nk_low_level_memset(naut, 0, sizeof(struct naut_info));

    // Initialize platform level interrupt controller for this HART
    plic_init();
    plic_init_hart();

    // Bring up UART device and printing so we can have output
    // uart_init();

    // Write supervisor trap vector location
    trap_init();

    printk(NAUT_WELCOME);

    // Setup per-core area for BSP
    w_tp((uint64_t)naut->sys.cpus[0]);

    // Setup the temporary boot-time allocator
    mm_boot_init((ulong_t) fdt);

    // Initialize boot CPU
    arch_early_init(naut);

    // /* this will finish up the identity map */
    // nk_paging_init(&(naut->sys.mem), mbd);

    // Setup the main kernel memory allocator
    nk_kmem_init();

    /* now we switch to the real kernel memory allocator, pages
     * allocated in the boot mem allocator are kept reserved */
    // mm_boot_kmem_init();

    // sti();

    while(1) {
        int c = uart_getchar();
        if (c != -1) {
            if (c == 13) printk("\n");
            else printk("%c", c);
        }
    }
}
