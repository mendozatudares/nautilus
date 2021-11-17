// SPDX-License-Identifier: GPL-2.0+
/*
 * SiFive UART driver
 * Copyright (C) 2018 Paul Walmsley <paul@pwsan.com>
 * Copyright (C) 2018-2019 SiFive
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Based partially on:
 * - drivers/tty/serial/pxa.c
 * - drivers/tty/serial/amba-pl011.c
 * - drivers/tty/serial/uartlite.c
 * - drivers/tty/serial/omap-serial.c
 * - drivers/pwm/pwm-sifive.c
 *
 * See the following sources for further documentation:
 * - Chapter 19 "Universal Asynchronous Receiver/Transmitter (UART)" of
 *   SiFive FE310-G000 v2p3
 * - The tree/master/src/main/scala/devices/uart directory of
 *   https://github.com/sifive/sifive-blocks/
 *
 * The SiFive UART design is not 8250-compatible.  The following common
 * features are not supported:
 * - Word lengths other than 8 bits
 * - Break handling
 * - Parity
 * - Flow control
 * - Modem signals (DSR, RI, etc.)
 * On the other hand, the design is free from the baggage of the 8250
 * programming model.
 */

#include <nautilus/nautilus.h>
#include <nautilus/devicetree.h>
#include <arch/riscv/sbi.h>
// #include <nautilus/dev.h>
// #include <nautilus/chardev.h>
#include <dev/sifive.h>

/*
 * Config macros
 */

/*
 * SIFIVE_SERIAL_MAX_PORTS: maximum number of UARTs on a device that can
 *                          host a serial console
 */
#define SIFIVE_SERIAL_MAX_PORTS			8

/*
 * SIFIVE_DEFAULT_BAUD_RATE: default baud rate that the driver should
 *                           configure itself to use
 */
#define SIFIVE_DEFAULT_BAUD_RATE		115200

/* SIFIVE_SERIAL_NAME: our driver's name that we pass to the operating system */
#define SIFIVE_SERIAL_NAME			"sifive-serial"

/* SIFIVE_TTY_PREFIX: tty name prefix for SiFive serial ports */
#define SIFIVE_TTY_PREFIX			"ttySIF"

/* SIFIVE_TX_FIFO_DEPTH: depth of the TX FIFO (in bytes) */
#define SIFIVE_TX_FIFO_DEPTH			8

/* SIFIVE_RX_FIFO_DEPTH: depth of the TX FIFO (in bytes) */
#define SIFIVE_RX_FIFO_DEPTH			8

#if (SIFIVE_TX_FIFO_DEPTH != SIFIVE_RX_FIFO_DEPTH)
#error Driver does not support configurations with different TX, RX FIFO sizes
#endif

enum {
  kUartSifiveTxwm = 1 << 0,
  kUartSifiveRxwm = 1 << 1,
};

union tx_data {
    struct {
    uint32_t data : 8;
    uint32_t reserved : 23;
    uint32_t isFull : 1;
    };
    uint32_t val;
};

union rx_data {
    struct {
    uint32_t data : 8;
    uint32_t reserved : 23;
    uint32_t isEmpty : 1;
    };
    uint32_t val;
};

union tx_ctrl {
    struct {
    uint32_t enable : 1;
    uint32_t nstop : 1;
    uint32_t reserved1 : 14;
    uint32_t cnt : 3;
    uint32_t reserved2 : 13;
    };
    uint32_t val;
};

union rx_ctrl {
    struct {
    uint32_t enable : 1;
    uint32_t reserved1 : 15;
    uint32_t cnt : 3;
    uint32_t reserved2 : 13;
    };
    uint32_t val;
};

struct sifive_serial_regs {
    union tx_data txdata;
    union rx_data rxdata;
    union tx_ctrl txctrl;
    union rx_ctrl rxctrl;

    uint32_t ie;  // interrupt enable
    uint32_t ip;  // interrupt pending
    uint32_t div;
    uint32_t unused;
};

inline struct sifive_serial_regs * regs(uint64_t addr) {
    return (struct sifive_serial_regs *)addr;
}

static addr_t uart_base = 0;
static uint64_t uart_clock = 0;
static uint8_t uart_inited = 0;

static void uart_init(addr_t base, uint64_t clock) {
    uart_base = base;
    uart_clock = clock;
    uint32_t baud = SIFIVE_DEFAULT_BAUD_RATE;
    uint64_t quotient = (clock + baud - 1) / baud;

    regs(uart_base)->div = (quotient == 0) ? 0 : (uint32_t)(quotient - 1);

    uart_inited = 1;
}

void serial_putchar(unsigned char ch) {
    if (!uart_inited) {
        sbi_call(SBI_CONSOLE_PUTCHAR, ch);
    } else {
        while (regs(uart_base)->txdata.isFull);
        regs(uart_base)->txdata.val = ch;
    }
}

int serial_getchar(void) {
    if (!uart_inited) {
        struct sbiret ret = sbi_call(SBI_CONSOLE_GETCHAR);
        return ret.error == -1 ? -1 : ret.value;
    } else {
        union rx_data data;
        data.val = regs(uart_base)->rxdata.val;
        return data.isEmpty ? -1 : data.data;
    }
}

void serial_write(const char *b) {
    while (b && *b) {
        serial_putchar(*b);
        b++;
    }
}

bool dtb_node_get_sifive_uart(struct dtb_node *n) {
    if (strstr(n->compatible, "sifive,uart0")) {
        uart_init(n->address, 5);
        return false;
    }
    return true;
}

void serial_init(void) {
    dtb_walk_devices(dtb_node_get_sifive_uart);
}

// TODO: integrate the sifive driver with nautilus, interrupt-based I/O
#if 0
/* This state supports early output */
/* the lock is reused for late output to serial lines / chars */
static spinlock_t sifive_lock; /* for SMP */
static uint8_t sifive_device_ready = 0;
static uint64_t sifive_io_addr;
static uint_t sifive_print_level;
static struct sifive_state *early_dev = 0;

/* The following state is for late output */

#define BUFSIZE 512

struct sifive_state {
    struct nk_char_dev *dev;
    uint64_t    addr;   // base address
    spinlock_t  input_lock;
    spinlock_t  output_lock;
    uint32_t    input_buf_head, input_buf_tail;
    uint8_t     input_buf[BUFSIZE];
    uint32_t    output_buf_head, output_buf_tail;
    uint8_t     output_buf[BUFSIZE];
};

static int sifive_do_get_characteristics(void * state, struct nk_char_dev_characteristics *c)
{
    memset(c,0,sizeof(*c));
    return 0;
}

static int sifive_input_empty(struct sifive_state *s)
{
    regs(s->addr)->rxctrl.
    return s->input_buf_head == s->input_buf_tail;
}

static int sifive_output_empty(struct sifive_state *s)
{
    return s->output_buf_head == s->output_buf_tail;
}

static int sifive_input_full(struct sifive_state *s)
{
    return ((s->input_buf_tail + 1) % BUFSIZE) == s->output_buf_head;
}

static int sifive_output_full(struct sifive_state *s)
{
    return ((s->output_buf_tail + 1) % BUFSIZE) == s->output_buf_head;
}

static void sifive_input_push(struct sifive_state *s, uint8_t data)
{
    s->input_buf[s->input_buf_tail] = data;
    s->input_buf_tail = (s->input_buf_tail + 1) % BUFSIZE;
}

static uint8_t sifive_input_pull(struct sifive_state *s)
{
    uint8_t temp = s->input_buf[s->input_buf_head];
    s->input_buf_head = (s->input_buf_head + 1) % BUFSIZE;
    return temp;
}

static void sifive_output_push(struct sifive_state *s, uint8_t data)
{
    s->output_buf[s->output_buf_tail] = data;
    s->output_buf_tail = (s->output_buf_tail + 1) % BUFSIZE;
}

static uint8_t sifive_output_pull(struct sifive_state *s)
{
    uint8_t temp = s->output_buf[s->output_buf_head];
    s->output_buf_head = (s->output_buf_head + 1) % BUFSIZE;
    return temp;
}

static int sifive_do_read(void *state, uint8_t *dest)
{
    struct sifive_state *s = (struct sifive_state *)state;

    int rc = -1;
    int flags;

    flags = spin_lock_irq_save(&s->input_lock);

    if (sifive_input_empty(s)) {
    rc = 0;
    goto out;
    }

    *dest = sifive_input_pull(s);
    rc = 1;

 out:
    spin_unlock_irq_restore(&s->input_lock, flags);
    return rc;
}

static int sifive_do_write(void *state, uint8_t *src)
{
    struct sifive_state *s = (struct sifive_state *)state;

    int rc = -1;
    int flags;

    flags = spin_lock_irq_save(&s->output_lock);

    if (sifive_output_full(s)) {
    rc = 0;
    goto out;
    }

    serial_output_push(s,*src);
    rc = 1;

 out:
    kick_output(s);
    spin_unlock_irq_restore(&s->output_lock, flags);
    return rc;
}

static void sifive_do_write_wait(void *state, uint8_t *src)
{
    while (sifive_do_write(state,src)==0) {}
}

static int sifive_do_status(void *state)
{
    struct sifive_state *s = (struct sifive_state *)state;

    int rc = 0;
    int flags;

    flags = spin_lock_irq_save(&s->input_lock);

    if (!sifive_input_empty(s)) {
    rc |= NK_CHARDEV_READABLE;
    }
    spin_unlock_irq_restore(&s->input_lock, flags);

    flags = spin_lock_irq_save(&s->output_lock);
    if (!sifive_output_full(s)) {
    rc |= NK_CHARDEV_WRITEABLE;
    }
    spin_unlock_irq_restore(&s->output_lock, flags);

    return rc;
}

static struct nk_char_dev_int chardevops = {
    .get_characteristics = sifive_do_get_characteristics,
    .read = sifive_do_read,
    .write = sifive_do_write,
    .status = sifive_do_status
};

static int sifive_do_get_characteristics(void * state, struct nk_char_dev_characteristics *c)
{
    memset(c,0,sizeof(*c));
    return 0;
}

static int serial_do_read(void *state, uint8_t *dest)
{
    struct serial_state *s = (struct serial_state *)state;

    int flags;

    flags = spin_lock_irq_save(&s->input_lock);

    int uart_getchar()

    

 out:
    spin_unlock_irq_restore(&s->input_lock, flags);
    return rc;
}

static struct nk_char_dev_int chardevops = {
    .get_characteristics = sifive_do_get_characteristics,
    .read = sifive_do_read,
    .write = sifive_do_write,
    .status = sifive_do_status
};

void
serial_early_init (void)
{
    serial_print_level = SERIAL_PRINT_DEBUG_LEVEL;

    spinlock_init(&serial_lock);


}

static int sifive_setup(struct sifive_state *s)
{

}

static void serial_putchar_early (uchar_t c)
{
    if (sifive_io_addr==0) {
        return;
    }

    if (!sifive_device_ready) {
        return;
    }

    int flags = spin_lock_irq_save(&sifive_lock);

    if (c == '\n') {
        
        /* wait for transmitter holding register ready */
        while (regs(sifive_io_addr)->txdata.isFull);

        /* send char */
        regs(sifive_io_addr)->txdata.val = '\r';

    }

    /* wait for transmitter holding register ready */
    while (regs(sifive_io_addr)->txdata.isFull);

    /* send char */
    regs(sifive_io_addr)->txdata.val = c;

    spin_unlock_irq_restore(&sifive_lock, flags);
}

void serial_putchar(uchar_t c)
{
    if (early_dev) {
    int flags = spin_lock_irq_save(&sifive_lock);
    if (c=='\n') {
        sifive_do_write_wait(early_dev,"\r");
    }
    sifive_do_write_wait(early_dev,&c);
    spin_unlock_irq_restore(&sifive_lock,flags);
    } else {
    sifive_putchar_early(c);
    }

}

void
serial_write (const char *buf)
{
    while (*buf) {
        serial_putchar(*buf);
        ++buf;
    }
}

uchar_t serial_getchar(void) {
    if (early_dev) {
        struct sbiret ret = sbi_call(SBI_CONSOLE_GETCHAR);
        return ret.error == -1 ? -1 : ret.value;
    } else {
        union rx_data data;
        data.val = regs()->rxdata.val;
        return data.isEmpty ? -1 : data.data;
    }
}

#endif