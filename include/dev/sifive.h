#ifndef __SIFIVE_H__
#define __SIFIVE_H__

#include <nautilus/naut_types.h>

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

void serial_init(void);
void serial_write(const char *b);
void serial_putchar(unsigned char ch);
int  serial_getchar(void);

#endif
