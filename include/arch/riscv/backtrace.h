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
#ifdef __BACKTRACE_H__

void 
nk_stack_dump (ulong_t n)
{
    void * rsp = NULL;

    asm volatile ("move %[_r], sp" : [_r] "=r" (rsp));

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

    printk("[-----------------------------------------------]\n");
}

#endif
