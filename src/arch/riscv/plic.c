#include <nautilus/naut_types.h>
#include <arch/riscv/riscv.h>
#include <arch/riscv/memlayout.h>
#include <arch/riscv/sbi.h>
#include <arch/riscv/cpu.h>

int boot_hart = -1;

void
plic_init_hart(void)
{
    int hart = 0;

    // Clear the "supervisor enable" field. This is the register that enables or disables external interrupts (UART, DISK, ETC)
    *(uint32_t*)PLIC_SENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

    // set this hart's S-mode priority threshold to 0.
    *(uint32_t*)PLIC_SPRIORITY(hart) = 0;

    if (boot_hart == -1) {
        boot_hart = hart;
    } else {
        *(uint32_t*)PLIC_SENABLE(hart) = PLIC_SENABLE(boot_hart);
    } 
}

void
plic_init(void)
{
    // set desired IRQ priorities non-zero (otherwise disabled).
    *(uint32_t*)(PLIC + UART0_IRQ*4) = 1;
    *(uint32_t*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
    return PLIC_SCLAIM(0);
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
    *(uint32_t*)PLIC_SCLAIM(0) = irq;
}

void
plic_enable(int irq, int priority) {
    *(uint32_t*)(PLIC + irq * 4) = 1;
    *(uint32_t*)PLIC_SPRIORITY(0) |= (1 << irq);
}

void
plic_disable(int irq) {
    *(uint32_t*)PLIC_SENABLE(0) &= ~(1 << irq);
}