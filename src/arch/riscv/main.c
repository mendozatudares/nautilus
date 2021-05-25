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

#include <nautilus/naut_types.h>

#define UART0 0x10000000L
#define Reg(reg) ((volatile unsigned char *)(UART0 + reg))
#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

#define RHR 0  // receive holding register (for input bytes)
#define THR 0  // transmit holding register (for output bytes)
#define IER 1  // interrupt enable register
#define IER_RX_ENABLE (1 << 0)
#define IER_TX_ENABLE (1 << 1)
#define FCR 2  // FIFO control register
#define FCR_FIFO_ENABLE (1 << 0)
#define FCR_FIFO_CLEAR (3 << 1)  // clear the content of the two FIFOs
#define ISR 2                    // interrupt status register
#define LCR 3                    // line control register
#define LCR_EIGHT_BITS (3 << 0)
#define LCR_BAUD_LATCH (1 << 7)  // special mode to set baud rate
#define LSR 5                    // line status register
#define LSR_RX_READY (1 << 0)    // input is waiting to be read from RHR
#define LSR_TX_IDLE (1 << 5)     // THR can accept another character to send

/* lowlevel.S, calls kerneltrap() */
extern void kernelvec(void);

uint64_t kernel_page_table[4096 / sizeof(uint64_t)] __attribute__((aligned(4096)));

void outb (unsigned char val)
{
    while ((ReadReg(LSR) & LSR_TX_IDLE) == 0);
    WriteReg(THR, val);
}

void print (char *b)
{
    while (b && *b) {
	outb(*b);
	b++;
    }
}

void uart (void) {
    WriteReg(IER, 0x00);
    WriteReg(LCR, LCR_BAUD_LATCH);
    WriteReg(0, 0x03);
    WriteReg(1, 0x00);
    WriteReg(LCR, LCR_EIGHT_BITS);
    WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);
}

void init (void)
{
    uart();
    __sync_synchronize();
    print("hello\n");
}

