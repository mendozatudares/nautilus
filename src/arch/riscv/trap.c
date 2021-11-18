#include <nautilus/cpu.h>
#include <nautilus/printk.h>
#include <arch/riscv/plic.h>
#include <arch/riscv/sbi.h>

void kernel_vec();

// set up to take exceptions and traps while in the kernel.
void trap_init_hart(void) { w_stvec((uint64_t)kernel_vec); }

/* Supervisor Trap Function */
void kernel_trap(struct nk_regs *regs) {
	static unsigned long ticks = 0;
  int which_dev = 0;
  regs->sepc = r_sepc();
  regs->status = r_sstatus();
  regs->cause = r_scause();
  regs->tval = r_stval();

  // if it was an interrupt, the top bit it set.
  int interrupt = (regs->cause >> 63);
  // the type of trap is the rest of the bits.
  int nr = regs->cause & ~(1llu << 63);

  if (interrupt) {
		if (nr == 5) {
		  // timer interrupt
			// on nautilus, we don't actaully want to set a new timer yet, we just
			// want to call into the scheduler, which schedules the next timer.
      // printk("tick %4d ", ++ticks);
      plic_timer_handler();
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
  w_sepc(regs->sepc);
  w_sstatus(regs->status);
}
