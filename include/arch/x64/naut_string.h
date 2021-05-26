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
#ifdef __STRING_H__

#define OP_T_THRES 8
#define OPSIZ sizeof(unsigned long int)

#define WORD_COPY_BWD(dst_ep, src_ep, nbytes_left, nbytes)            \
  do                                          \
    {                                         \
      int __d0;                                   \
      asm volatile(/* Set the direction flag, so copying goes backwards.  */  \
           "std\n"                            \
           /* Copy longwords.  */                     \
           "rep\n"                            \
           "movsl\n"                              \
           /* Clear the dir flag.  Convention says it should be 0. */ \
           "cld" :                            \
           "=D" (dst_ep), "=S" (src_ep), "=c" (__d0) :            \
           "0" (dst_ep - 4), "1" (src_ep - 4), "2" ((nbytes) / 4) :   \
           "memory");                             \
      dst_ep += 4;                                \
      src_ep += 4;                                \
      (nbytes_left) = (nbytes) % 4;                       \
    } while (0)

#define BYTE_COPY_BWD(dst_ep, src_ep, nbytes)                     \
  do                                          \
    {                                         \
      int __d0;                                   \
      asm volatile(/* Set the direction flag, so copying goes backwards.  */  \
           "std\n"                            \
           /* Copy bytes.  */                         \
           "rep\n"                            \
           "movsb\n"                              \
           /* Clear the dir flag.  Convention says it should be 0. */ \
           "cld" :                            \
           "=D" (dst_ep), "=S" (src_ep), "=c" (__d0) :            \
           "0" (dst_ep - 1), "1" (src_ep - 1), "2" (nbytes) :         \
           "memory");                             \
      dst_ep += 1;                                \
      src_ep += 1;                                \
    } while (0)

#endif 
