#ifndef __PLIC_H__
#define __PLIC_H__

#define VIRTIO0_IRQ 1
#define UART0_IRQ 10

void plic_init_hart(void);
void plic_init(void);
int  plic_claim(void);
void plic_complete(int irq);
void plic_enable(int irq, int priority);
void plic_disable(int irq);

#endif