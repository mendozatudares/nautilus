#ifndef __SIFIVE_H__
#define __SIFIVE_H__

#include <nautilus/naut_types.h>

void sifive_uart_init(void);
void uart_puts(char *b);
void uart_putchar(char ch);
int  uart_getchar(void);

#endif