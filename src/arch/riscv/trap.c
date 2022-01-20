#include <nautilus/arch.h>
#include <nautilus/printk.h>
#include <arch/riscv/plic.h>

void kernel_vec();

// set up to take exceptions and traps while in the kernel.
void trap_init_hart(void) { write_csr(stvec,(uint64_t)kernel_vec); }

/* Supervisor Trap Function */
void kernel_trap(struct nk_regs *regs) {
  static unsigned long ticks = 0;
  int which_dev = 0;
  regs->sepc = read_csr(sepc);
  regs->status = read_csr(sstatus);
  regs->cause = read_csr(scause);
  regs->tval = read_csr(stval);

  // if it was an interrupt, the top bit it set.
  int interrupt = (regs->cause >> 63);
  // the type of trap is the rest of the bits.
  int nr = regs->cause & ~(1llu << 63);

  if (interrupt) {
    if (nr == 5) {
      // timer interrupt
      // on nautilus, we don't actaully want to set a new timer yet, we just
      // want to call into the scheduler, which schedules the next timer.
      /* printk("tick %4d ", ++ticks); */
      arch_timer_handler(0,0,0);
    } else if (nr == 9) {
      // supervisor external interrupt
      // first, we claim the irq from the PLIC
      int irq = plic_claim();

      // do something with the IRQ

      // the PLIC allows each device to raise at most one
      // interrupt at a time; tell the PLIC the device is
      // now allowed to interrupt again.
      if (irq) plic_complete(irq);
    }
  } else {
    // it's a fault/trap (like illegal instruction)
    if (nr == 8) {
      printk("Unhandled syscall\n");
    } else {
      panic("\n+++ Unhandled Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n", regs->cause, regs->sepc, regs->tval);
    }
  }

  // restore trap registers for use by kernelvec.S's sepc instruction.
  write_csr(sepc, regs->sepc);
  write_csr(sstatus, regs->status);
}
