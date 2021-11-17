#ifndef __SIFIVE_H__
#define __SIFIVE_H__

#include <nautilus/naut_types.h>

void serial_init(void);
void serial_write(const char *b);
void serial_putchar(unsigned char ch);
int  serial_getchar(void);

#endif