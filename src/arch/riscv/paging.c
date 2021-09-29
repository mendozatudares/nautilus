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
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/paging.h>
#include <nautilus/naut_string.h>
#include <nautilus/mb_utils.h>
#include <nautilus/idt.h>
#include <nautilus/cpu.h>
#include <nautilus/errno.h>
#include <nautilus/cpuid.h>
#include <nautilus/backtrace.h>
#include <nautilus/macros.h>
#include <nautilus/naut_assert.h>
#include <nautilus/numa.h>
#include <nautilus/mm.h>
#include <lib/bitmap.h>
#include <nautilus/percpu.h>

#ifdef NAUT_CONFIG_XEON_PHI
#include <nautilus/sfi.h>
#endif

#ifdef NAUT_CONFIG_HVM_HRT
#include <arch/hrt/hrt.h>
#endif

#ifdef NAUT_CONFIG_ASPACES
#include <nautilus/aspace.h>
#endif

#ifndef NAUT_CONFIG_DEBUG_PAGING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#include <arch/riscv/riscv.h>
#include <arch/riscv/memlayout.h>

extern uint8_t boot_mm_inactive;

extern ulong_t kernel_page_table;

extern ulong_t etext;


static char * ps2str[3] = {
    [PS_4K] = "4KB",
    [PS_2M] = "2MB",
    [PS_1G] = "1GB",
};


extern uint8_t cpu_info_ready;

/*
 * align_addr
 *
 * align addr *up* to the nearest align boundary
 *
 * @addr: address to align
 * @align: power of 2 to align to
 *
 * returns the aligned address
 *
 */
static inline ulong_t
align_addr (ulong_t addr, ulong_t align)
{
    ASSERT(!(align & (align-1)));
    return (~(align - 1)) & (addr + align);
}


static inline int
gig_pages_supported (void)
{
    return 0;
}


static page_size_t
largest_page_size (void)
{
    if (gig_pages_supported()) {
        return PS_1G;
    }

    return PS_2M;
}


static ulong_t *
walk(ulong_t * pml, addr_t addr, int alloc)
{
    if(addr >= MAXVA) {
        panic("walk");
    }

    for(int level = 2; level > 0; level--) {
        pte_t *pte = &pml[PX(level, addr)];
        if(*pte & PTE_V) {
            pml = (ulong_t *) PTE2PA(*pte);
        } else {
            if(!alloc || (pml = (ulong_t*) mm_boot_alloc_aligned(PAGE_SIZE_4KB, PAGE_SIZE_4KB)) == 0)
                return 0;
            memset(pml, 0, PGSIZE);
            *pte = PA2PTE(pml) | PTE_V;
        }
    }
    return &pml[PX(0, addr)];
}


int
map_page_range (ulong_t * pml, addr_t vaddr, addr_t paddr, uint64_t size, uint64_t flags)
{
    uint64_t a, last;
    ulong_t *pte;

    a = PGROUNDDOWN(vaddr);
    last = PGROUNDDOWN(vaddr + size - 1);
    for (;;) {
        if((pte = walk(pml, a, 1)) == 0)
            return -1;
        if(*pte & PTE_V)
            panic("remap");
        *pte = PA2PTE(paddr) | flags | PTE_V;
        if(a == last)
            break;
        a += PGSIZE;
        paddr += PGSIZE;
    }
    return 0;
}


static void
construct_ident_map (ulong_t * pml, ulong_t bytes)
{
    // uart registers
    map_page_range(pml, UART0, UART0, PAGE_SIZE_4KB, PTE_R | PTE_W);

    // virtio mmio disk interface
    map_page_range(pml, VIRTIO0, VIRTIO0, PAGE_SIZE_4KB, PTE_R | PTE_W);

    // PLIC
    map_page_range(pml, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

    uint64_t kernend = (uint64_t)&etext;

    // map kernel text executable, writable, and readable.
    map_page_range(pml, KERNBASE, KERNBASE, kernend-KERNBASE, PTE_R | PTE_X | PTE_W);

    // map kernel data and the physical RAM we'll make use of.
    map_page_range(pml, kernend, kernend, PHYSTOP-kernend, PTE_R | PTE_X | PTE_W);
}


/*
 * nk_pf_handler
 *
 * page fault handler
 *
int
nk_pf_handler (excp_entry_t * excp,
               excp_vec_t     vector,
               void         * state)
{

    cpu_id_t id = cpu_info_ready ? my_cpu_id() : 0xffffffff;
    uint64_t fault_addr = read_cr2();

#ifdef NAUT_CONFIG_HVM_HRT
    if (excp->error_code == UPCALL_MAGIC_ERROR) {
        return nautilus_hrt_upcall_handler(NULL, 0);
    }
#endif

#ifdef NAUT_CONFIG_ASPACES
    if (!nk_aspace_exception(excp,vector,state)) {
	return 0;
    }
#endif

#ifdef NAUT_CONFIG_ENABLE_MONITOR
    int nk_monitor_excp_entry(excp_entry_t * excp,
			      excp_vec_t vector,
			      void *state);

    return nk_monitor_excp_entry(excp, vector, state);
#endif

    printk("\n+++ Page Fault +++\n"
            "RIP: %p    Fault Address: 0x%llx \n"
            "Error Code: 0x%x    (core=%u)\n",
            (void*)excp->rip,
            fault_addr,
            excp->error_code,
            id);

    struct nk_regs * r = (struct nk_regs*)((char*)excp - 128);
    nk_print_regs(r);
    backtrace(r->rbp);

    panic("+++ HALTING +++\n");
    return 0;
}
 */


/*
 * nk_gpf_handler
 *
 * general protection fault handler
 *
int
nk_gpf_handler (excp_entry_t * excp,
		excp_vec_t     vector,
		void         * state)
{

    cpu_id_t id = cpu_info_ready ? my_cpu_id() : 0xffffffff;

#ifdef NAUT_CONFIG_ASPACES
    if (!nk_aspace_exception(excp,vector,state)) {
	return 0;
    }
#endif

    // if monitor is active, we will fall through to it
    // by calling null_excp_handler
    return null_excp_handler(excp,vector,state);
}
 */

static uint64_t default_satp;

/*
 * Identity map all of physical memory using
 * the largest pages possible
 */
static void
kern_ident_map (struct nk_mem_info * mem, ulong_t mbd)
{
    page_size_t lps  = PS_4K;
    ulong_t last_pfn = mm_boot_last_pfn();
    ulong_t ps       = ps_type_to_size(lps);
    pml4e_t * pml    = NULL;

    /* create a new PML4 */
    pml = mm_boot_alloc_aligned(PAGE_SIZE_4KB, PAGE_SIZE_4KB);
    if (!pml) {
        ERROR_PRINT("Could not allocate new PML4\n");
        return;
    }
    memset(pml, 0, PAGE_SIZE_4KB);

    printk("Remapping phys mem [%p - %p] with %s pages\n",
            (void*)0,
            (void*)(last_pfn<<PAGE_SHIFT),
            ps2str[lps]);

    construct_ident_map(pml, last_pfn<<PAGE_SHIFT);

    kernel_page_table = (ulong_t) pml;

    default_satp = (ulong_t) MAKE_SATP(pml);

    /* install the new tables, this will also flush the TLB */
    w_satp(MAKE_SATP(pml));
    sfence_vma();

}

uint64_t nk_paging_default_page_size()
{
    return ps_type_to_size(largest_page_size());
}

uint64_t nk_paging_default_satp()
{
    return default_satp;
}


void
nk_paging_init (struct nk_mem_info * mem, ulong_t mbd)
{
    kern_ident_map(mem, mbd);
}
