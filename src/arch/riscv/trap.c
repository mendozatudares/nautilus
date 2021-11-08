#include <nautilus/cpu.h>
#include <nautilus/printk.h>
#include <arch/riscv/plic.h>

void kernel_vec();

// set up to take exceptions and traps while in the kernel.
void
trap_init_hart(void)
{
    w_stvec((uint64_t)kernel_vec);
}

void uart_intr(void);

int
dev_intr(void)
{
    uint64_t scause = r_scause();

    if ((scause & 0x8000000000000000L) && (scause & 0xff) == 9){
        // this is a supervisor external interrupt, via PLIC.

        // irq indicates which device interrupted.
        int irq = plic_claim();

        if (irq == UART0_IRQ){
            uart_intr();
        } else if (irq) {
            printk("Unexpected interrupt IRQ=%d\n", irq);
        }

        // the PLIC allows each device to raise at most one
        // interrupt at a time; tell the PLIC the device is
        // now allowed to interrupt again.
        if (irq) plic_complete(irq);

        return 1;
    } else if (scause == 0x8000000000000001L){
        // software interrupt from a machine-mode timer interrupt,
        // forwarded by timervec in kernelvec.S.

        printk("\n+++ Timer Interrupt +++\nsepc: %p\ntime: %p\n", r_sepc(), r_time());

        // acknowledge the software interrupt by clearing
        // the SSIP bit in sip.
        w_sip(r_sip() & ~2);

        return 2;
    } else {
        return 0;
    }
}

/* Supervisor Trap Function */
void
kernel_trap()
{
    int which_dev = 0;
    uint64_t sepc = r_sepc();
    uint64_t sstatus = r_sstatus();
    uint64_t scause = r_scause();
    printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n", scause, sepc, r_stval());

    if ((sstatus & SSTATUS_SPP) == 0) {
        // printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n", scause, sepc, r_stval());
        panic("Not from supervisor mode\n");
    }
    if (intr_get() != 0) {
        // printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n", scause, sepc, r_stval());
        panic("Interrupts enabled\n");
    }
    if ((which_dev = dev_intr()) == 0){
        // printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n", scause, sepc, r_stval());
        panic("Unhandled trap\n");
    }

    // give up the CPU if this is a timer interrupt.
    if(which_dev == 2);
        // yield();

    // the yield() may have caused some traps to occur,
    // so restore trap registers for use by kernelvec.S's sepc instruction.
    w_sepc(sepc);
    w_sstatus(sstatus);
}
