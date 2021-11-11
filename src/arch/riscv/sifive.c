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

#include <nautilus/naut_types.h>
#include <nautilus/devicetree.h>
#include <arch/riscv/sbi.h>

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


static addr_t uart_base = 0;
static uint64_t uart_clock = 0;
static uint8_t uart_inited = 0;

inline struct sifive_serial_regs * regs() {
    return (struct sifive_serial_regs *)uart_base;
}

void uart_init(addr_t base, uint64_t clock) {
    uart_base = base;
    uart_clock = clock;
    uint32_t baud = SIFIVE_DEFAULT_BAUD_RATE;
    uint64_t quotient = (clock + baud - 1) / baud;

    regs()->div = (quotient == 0) ? 0 : (uint32_t)(quotient - 1);

    uart_inited = 1;
}

void uart_putchar(char ch) {
    if (!uart_inited) {
        sbi_call(SBI_CONSOLE_PUTCHAR, ch);
    } else {
        while (regs()->txdata.isFull);
        regs()->txdata.val = ch;
    }
}

int uart_getchar_wait(uint8_t wait) {
    if (!uart_inited) {
        struct sbiret ret;
        do {
            ret = sbi_call(SBI_CONSOLE_GETCHAR);
        } while (!wait || ret.error != -1);
        return ret.error == -1 ? -1 : ret.value;
    } else {
        union rx_data data;
        do {
            data.val = regs()->rxdata.val;
        } while (!wait || data.isEmpty);
        return data.isEmpty ? -1 : data.data;
    }
}

void uart_puts(char *b) {
    while (b && *b) {
        uart_putchar(*b);
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

void sifive_uart_init(void) {
    dtb_walk_devices(dtb_node_get_sifive_uart);
}