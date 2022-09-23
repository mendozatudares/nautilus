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
#include <nautilus/intrinsics.h>
#include <nautilus/nautilus.h>
#include <nautilus/cpu.h>
#include <nautilus/thread.h>
#ifdef NAUT_CONFIG_PROVENANCE
#include <nautilus/provenance.h>
#endif

extern int printk (const char * fmt, ...);

#ifdef NAUT_CONFIG_PROVENANCE
static void print_prov_info(uint64_t addr) {
	provenance_info* prov_info = nk_prov_get_info(addr);
	if (prov_info != NULL) {
		printk("Symbol: %s   ", (prov_info->symbol != NULL) ? (char*) prov_info->symbol : "???");
		printk("Section: %s\n", (prov_info->section != NULL) ? (char*) prov_info->section : "???");
		if (prov_info->line_info != NULL) {
			// TODO: print line info
		}
		if (prov_info->file_info != NULL) {
			// TODO: print file info
		}
		free(prov_info);
	}
}
#endif

void __attribute__((noinline))
__do_backtrace (void ** fp, unsigned depth)
{
    if (!fp || fp >= (void**)nk_get_nautilus_info()->sys.mem.phys_mem_avail) {
        return;
    }
    
    printk("[%2u] RIP: %p RBP: %p\n", depth, *(fp+1), *fp);
#ifdef NAUT_CONFIG_PROVENANCE
	print_prov_info((uint64_t) *(fp+1));
#endif
    __do_backtrace(*fp, depth+1);
}

/*
 * dump memory in 16 byte chunks
 */
void 
nk_dump_mem (const void * addr, ulong_t n)
{
    int i, j;
    ulong_t new = (n % 16 == 0) ? n : ((n+16) & ~0xf);

    for (i = 0; i < new/(sizeof(void*)); i+=2) {
        printk("%p: %016llx  %016llx  ", ((void**)addr + i), *((void**)addr + i), *((void**)addr + i + 1));
        for (j = 0; j < 16; j++) {
            char tmp = *((char*)addr + j);
            printk("%c", (tmp < 0x7f && tmp > 0x1f) ? tmp : '.');
        }

        printk("\n");
    }
}

void 
nk_stack_dump (ulong_t n)
{
    void * rsp = NULL;

    rsp = arch_read_sp();

    if (!rsp) {
        return;
    }

    printk("Stack Dump:\n");

    nk_dump_mem(rsp, n);
}


void 
nk_print_regs (struct nk_regs * r)
{

    printk("Current Thread=0x%x (%p) \"%s\"\n", 
            get_cur_thread() ? get_cur_thread()->tid : -1,
            get_cur_thread() ? (void*)get_cur_thread() :  NULL,
            !get_cur_thread() ? "NONE" : get_cur_thread()->is_idle ? "*idle*" : get_cur_thread()->name);

    
    printk("[-------------- Register Contents --------------]\n");

    arch_print_regs(r);

    printk("[-----------------------------------------------]\n");
}
