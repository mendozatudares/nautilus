#ifndef __PLIC_H__
#define __PLIC_H__

void plic_init_hart(int hart);
void plic_init(void);
int  plic_claim(void);
void plic_complete(int irq);
void plic_enable(int irq, int priority);
void plic_disable(int irq);
void plic_timer_handler(void);

#endif
