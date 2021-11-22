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

#ifndef NAUT_CONFIG_DEBUG_PAGING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

#define PTE_V (1L << 0) // valid
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4) // 1 -> user can access
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)

#define PTE_FLAGS(pte) ((pte) & 0x3FF)

// extract the three 9-bit page table indices from a virtual address.
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64_t) (va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))

#define SATP_FLAG  8ULL
#define SATP_SHIFT 60
#define SATP_MODE (SATP_FLAG << SATP_SHIFT)
#define MAKE_SATP(page_table) (SATP_MODE | (((uint64_t)page_table) >> 12))

static void
__construct_tables_1g (pml4e_t * pml, ulong_t bytes)
{
    ulong_t npages = (bytes + PAGE_SIZE_1GB - 1)/PAGE_SIZE_1GB;
    ulong_t filled_pgs = 0;
    unsigned i;
    ulong_t addr = 0;

    for (i = 0; i < NUM_PML4_ENTRIES && filled_pgs < npages; i++) {
        pte_t * pte = (pte_t*)(pml + i);

        *pte = PA2PTE(addr) | PTE_R | PTE_W | PTE_X | PTE_V | PTE_A | PTE_D;
        DEBUG_PRINT("  pte[%d] at %p = %p\n", i, pte, *pte);

        ++filled_pgs;
        addr += PAGE_SIZE_1GB;
    }
}

static void
construct_ident_map (pml4e_t * pml, page_size_t ptype, ulong_t bytes)
{
    if (ptype != PS_1G) {
        ERROR_PRINT("Expected page type PS_1G (received %u)\n", ptype);
        return;
    }
    ulong_t ps = ps_type_to_size(ptype);

    __construct_tables_1g(pml, bytes);
}

static uint64_t default_satp;

/*
 * Identity map all of physical memory using
 * the largest pages possible (1GB)
 */
static void
kern_ident_map (struct nk_mem_info * mem, ulong_t fdt)
{
    page_size_t lps  = PS_1G;
    ulong_t last_pfn = mm_boot_last_pfn();
    ulong_t ps       = ps_type_to_size(lps);
    pml4e_t * pml    = NULL;

    /* create a new PML4 */
    pml = mm_boot_alloc_aligned(PAGE_SIZE_4KB, PAGE_SIZE_4KB);
    if (!pml) {
        ERROR_PRINT("Could not allocate new PML4");
        return;
    }
    memset(pml, 0, PAGE_SIZE_4KB);

    printk("Remapping phys mem [%p - %p] with 1G pages\n",
            (void*)0,
            (void*)(last_pfn<<PAGE_SHIFT));

    construct_ident_map(pml, lps, last_pfn<<PAGE_SHIFT_2MB);

    default_satp = (ulong_t)MAKE_SATP(pml);

    /* intall the new table, we should also flush the tlb */
    write_csr(satp, default_satp);
    sfence_vma();
}

uint64_t nk_paging_default_satp()
{
    return default_satp;
}

void
nk_paging_init (struct nk_mem_info * mem, ulong_t fdt)
{
    kern_ident_map(mem, fdt);
}
