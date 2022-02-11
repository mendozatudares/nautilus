#include <nautilus/arch.h>
#include <nautilus/printk.h>
#include <arch/riscv/plic.h>

void kernel_vec();

// set up to take exceptions and traps while in the kernel.
void trap_init(void) { write_csr(stvec,(uint64_t)kernel_vec); }

static void print_regs(struct nk_regs *r) {
  int i = 0;
  printk("Current Thread=0x%x (%p) \"%s\"\n",
      get_cur_thread() ? get_cur_thread()->tid : -1,
      get_cur_thread() ? (void*)get_cur_thread() :  NULL,
      !get_cur_thread() ? "NONE" : get_cur_thread()->is_idle ? "*idle*" : get_cur_thread()->name);

  printk("[-------------- Register Contents --------------]\n");
  printk("RA:  %016lx SP:  %016lx\n", r->ra, r->sp);
  printk("GP:  %016lx TP:  %016lx\n", r->gp, r->tp);
  printk("T00: %016lx T01: %016lx T02: %016lx\n", r->t0, r->t1, r->t2);
  printk("S00: %016lx S01: %016lx A00: %016lx\n", r->s0, r->s1, r->a0);
  printk("A01: %016lx A02: %016lx A03: %016lx\n", r->a1, r->a2, r->a3);
  printk("A04: %016lx A05: %016lx A06: %016lx\n", r->a4, r->a5, r->a6);
  printk("A07: %016lx S02: %016lx S03: %016lx\n", r->a7, r->s2, r->s3);
  printk("S04: %016lx S05: %016lx S06: %016lx\n", r->s4, r->s5, r->s6);
  printk("S07: %016lx S08: %016lx S09: %016lx\n", r->s7, r->s8, r->s9);
  printk("S10: %016lx S11: %016lx T03: %016lx\n", r->s10, r->s11, r->t3);
  printk("T04: %016lx T05: %016lx T06: %016lx\n", r->t4, r->t5, r->t6);

  printk("[-----------------------------------------------]\n");
}

static void kernel_unhandled_trap(struct nk_regs *regs, const char *type) {
  printk("===========================================================================================\n");
  printk("+++ Unhandled Trap: %s +++\n", type);
  printk("SEPC:   %016lx CAUSE:   %016lx TVAL: %016lx\n", regs->sepc, regs->cause, regs->tval);
  printk("STATUS: %016lx SCRATCH: %016lx\n", regs->status, regs->scratch);
  print_regs(regs);
  printk("===========================================================================================\n");
  panic("Halting hart!\n");
}

/* Supervisor Trap Function */
void kernel_trap(struct nk_regs *regs) {
  regs->sepc = read_csr(sepc);
  regs->status = read_csr(sstatus);
  regs->tval = read_csr(stval);
  regs->cause = read_csr(scause);
  regs->scratch = read_csr(sscratch);

  // if it was an interrupt, the top bit it set.
  int interrupt = (regs->cause >> 63);
  // the type of trap is the rest of the bits.
  int nr = regs->cause & ~(1llu << 63);

  if (interrupt) {
      printk("int!, nr = %d, sie=%p, pending=%p\n", nr, read_csr(sie), plic_pending());
    if (nr == 5) {
      // timer interrupt
      // on nautilus, we don't actaully want to set a new timer yet, we just
      // want to call into the scheduler, which schedules the next timer.
      arch_timer_handler(0,0,0);
    } else if (nr == 9) {
      // supervisor external interrupt
      // first, we claim the irq from the PLIC
      int irq = plic_claim();

      // do something with the IRQ
      panic("received irq: %d\n", irq);

      // the PLIC allows each device to raise at most one
      // interrupt at a time; tell the PLIC the device is
      // now allowed to interrupt again.
      if (irq) plic_complete(irq);
    }
  } else {
    // it's a fault/trap (like illegal instruction)
    switch (nr) {
      case 0:
        kernel_unhandled_trap(regs, "Instruction address misaligned");
        break;
      case 1:
        kernel_unhandled_trap(regs, "Instruction access fault");
        break;
      case 2:
        kernel_unhandled_trap(regs, "Illegal instruction");
        break;
      case 3:
        kernel_unhandled_trap(regs, "Breakpoint");
        break;
      case 4:
        kernel_unhandled_trap(regs, "Load address misaligned");
        break;
      case 5:
        kernel_unhandled_trap(regs, "Load access fault");
        break;
      case 6:
        kernel_unhandled_trap(regs, "Store/AMO address misaligned");
        break;
      case 7:
        kernel_unhandled_trap(regs, "Store/AMO access fault");
        break;
      case 8:
        kernel_unhandled_trap(regs, "Environment call from U-mode");
        break;
      case 9:
        kernel_unhandled_trap(regs, "Environment call from S-mode");
        break;
      case 12:
        kernel_unhandled_trap(regs, "Instruction page fault");
        break;
      case 13:
        kernel_unhandled_trap(regs, "Load page fault");
        break;
      case 15:
        kernel_unhandled_trap(regs, "Store/AMO page fault");
        break;
      default:
        kernel_unhandled_trap(regs, "Reserved");
        break;
    }
  }

  // restore trap registers for use by kernelvec.S's sepc instruction.
  write_csr(sepc, regs->sepc);
  write_csr(sstatus, regs->status);
}
