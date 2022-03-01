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
 * http://xtack.sandia.gov/hobbes
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
#include <nautilus/idt.h>
#include <nautilus/irq.h>
#include <nautilus/cpu.h>
#include <nautilus/mm.h>

#define MAX_IRQ_NUM    15     // this is really PIC-specific


/* NOTE: the APIC organizes interrupt priorities as follows:
 * class 0: interrupt vectors 0-15
 * class 1: interrupt vectors 16-31
 * .
 * .
 * .
 * class 15: interrupt vectors 0x1f - 0xff
 *
 * The upper 4 bits indicate the priority class
 * and the lower 4 represent the int number within
 * that class
 *
 * higher class = higher priority
 *
 * higher vector = higher priority within the class
 *
 * Because x86 needs the vectors 0-31, the first
 * two classes are reserved
 *
 * In short, we use vectors above 31, with increasing
 * priority with increasing vector number
 *
 */


uint8_t
irq_to_vec (uint8_t irq)
{
    return nk_get_nautilus_info()->sys.int_info.irq_map[irq].vector;
}

/* 
 * this should only be used when the OS interrupt vector
 * is known ahead of time, that is, *not* in the case
 * of external interrupts. It's much more likely you
 * should be using register_irq_handler
 */
int 
register_int_handler (uint16_t int_vec, 
                      int (*handler)(excp_entry_t *, excp_vec_t, void *),
                      void * priv_data)
{

    if (!handler) {
        ERROR_PRINT("Attempt to register interrupt %d with invalid handler\n", int_vec);
        return -1;
    }

    if (int_vec > 0xff) {
        ERROR_PRINT("Attempt to register invalid interrupt(0x%x)\n", int_vec);
        return -1;
    }

    idt_assign_entry(int_vec, (ulong_t)handler, (ulong_t)priv_data);

    return 0;
}


int 
register_irq_handler (uint16_t irq, 
                      int (*handler)(excp_entry_t *, excp_vec_t, void *),
                      void * priv_data)
{
    uint8_t int_vector;

    if (!handler) {
        ERROR_PRINT("Attempt to register IRQ %d with invalid handler\n", irq);
        return -1;
    }

    if (irq > MAX_IRQ_NUM) {
        ERROR_PRINT("Attempt to register invalid IRQ (0x%x)\n", irq);
        return -1;
    }

    int_vector = irq_to_vec(irq);

    idt_assign_entry(int_vector, (ulong_t)handler, (ulong_t)priv_data);

    return 0;
}

