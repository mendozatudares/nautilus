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
#ifdef __PER_CPU_H__

#define __movop_4 lw
#define __movop_8 ld
#define __cmpop_4 amoswap.w
#define __cmpop_8 amoswap.d

#define __areg a0

#define __xpand_str(x) __stringify(x)
#define __stringify(x) #x
#define __percpu_seg tp

#include <nautilus/nautilus.h>

#define __per_cpu_get(var, n)                                        \
    ({                                                               \
    typeof(((struct cpu*)0)->var) __r;                               \
    asm volatile (__xpand_str(__movop_##n) " %[_r], %[_o]("__xpand_str(__percpu_seg)")"    \
                  : [_r] "=r" (__r)                                  \
                  : [_o] "n"  (offsetof(struct cpu, var)));          \
    __r;                                                             \
    })


/* KCH NOTE: var needs to be in the cpu struct */
#define per_cpu_get(var)           \
    ({                      \
     typeof(((struct cpu*)0)->var) __r; \
     switch(sizeof(__r)) { \
        case 4: \
            __r = (typeof(__r)) __per_cpu_get(var,4); \
            break; \
        case 8: \
            __r = (typeof(__r)) __per_cpu_get(var,8);\
            break;\
        default: \
            printk("ERROR: undefined op size in per_cpu_var\n");\
        } \
        __r; })


#define __per_cpu_put(var, newval, n) \
do {\
    asm volatile (__xpand_str(__cmpop_##n) " zero, %[_v], %[_o]("__xpand_str(__percpu_seg)")" \
                   : /* no outputs */                                      \
                   : [_o] "n" (offsetof(struct cpu, var)),                 \
                     [_v] "r" (newval)                                     \
                   : "zero", "memory");                                    \
} while (0)


#define per_cpu_put(var, newval)                                      \
do { \
     typeof(&((struct cpu*)0)->var) __r; \
     switch (sizeof(__r)) { \
         case 4: \
            __per_cpu_put(var,newval,4); \
            break; \
         case 8: \
            __per_cpu_put(var,newval,8); \
            break; \
        default: \
            printk("ERROR: undefined op size in per_cpu_put\n"); \
    } \
     } while (0)


#define my_cpu_id() per_cpu_get(id)

#include <nautilus/smp.h>
static inline struct cpu*
get_cpu (void)
{
    uint64_t x;
    asm volatile("mv %0, tp" : "=r" (x) );
    return (struct cpu*) x;
}

#endif
