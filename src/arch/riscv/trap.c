#include <arch/riscv/plic.h>
#include <nautilus/cpu.h>
#include <nautilus/printk.h>


void kernel_vec();

// set up to take exceptions and traps while in the kernel.
void trap_init_hart(void) { w_stvec((uint64_t)kernel_vec); }

void uart_intr(void);

int dev_intr(void) {
  uint64_t scause = r_scause();

  if ((scause & 0x8000000000000000L) && (scause & 0xff) == 9) {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ) {
      uart_intr();
    } else if (irq) {
      printk("Unexpected interrupt IRQ=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq) plic_complete(irq);

    return 1;
  } else if (scause == 0x8000000000000001L) {
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    printk("\n+++ Timer Interrupt +++\nsepc: %p\ntime: %p\n", r_sepc(),
	   r_time());

    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

#define RISCV_CLOCKS_PER_SECOND 1000000
#define TICK_INTERVAL (RISCV_CLOCKS_PER_SECOND / NAUT_CONFIG_HZ)

static inline uint64_t get_time() {
  uint64_t x;
  asm volatile("csrr %0, time" : "=r"(x));
  return x;
}

struct regs {
  uint64_t ra; /* x1: Return address */
  uint64_t sp; /* x2: Stack pointer */
  uint64_t gp; /* x3: Global pointer */
  uint64_t tp; /* x4: Thread Pointer */
  uint64_t t0; /* x5: Temp 0 */
  uint64_t t1; /* x6: Temp 1 */
  uint64_t t2; /* x7: Temp 2 */
  uint64_t s0; /* x8: Saved register / Frame Pointer */
  uint64_t s1; /* x9: Saved register */
  uint64_t a0; /* Arguments, you get it :) */
  uint64_t a1;
  uint64_t a2;
  uint64_t a3;
  uint64_t a4;
  uint64_t a5;
  uint64_t a6;
  uint64_t a7;
  uint64_t s2; /* More Saved registers... */
  uint64_t s3;
  uint64_t s4;
  uint64_t s5;
  uint64_t s6;
  uint64_t s7;
  uint64_t s8;
  uint64_t s9;
  uint64_t s10;
  uint64_t s11;
  uint64_t t3; /* More temporaries */
  uint64_t t4;
  uint64_t t5;
  uint64_t t6;

  /* Exception PC */
  uint64_t sepc;    /* 31 */
  uint64_t status;  /* 32 */
  uint64_t tval;    /* 33 */
  uint64_t cause;   /* 34 */
  uint64_t scratch; /* 35 */
  /* Missing floating point registers in the kernel trap frame */
};

/* Supervisor Trap Function */
void kernel_trap(struct regs *regs) {
	static unsigned long ticks = 0;
  int which_dev = 0;
  uint64_t sepc = r_sepc();
  uint64_t sstatus = r_sstatus();
  uint64_t scause = r_scause();

  // printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n", scause, sepc, r_stval());

  // if it was an interrupt, the top bit it set.
  int interrupt = (scause >> 63);
  // the type of trap is the rest of the bits.
  int nr = scause & ~(1llu << 63);

  if (interrupt) {
		// timer interrupt
		if (nr == 5) {
			// on nautilus, we don't actaully want to set a new timer yet, we just
			// want to call into the scheduler, which schedules the next timer.
			// TODO: do that
			printk("got a timer %lu\n", ticks++);
  		sbi_set_timer(get_time() + TICK_INTERVAL);
		}
  } else {
    // it's a fault/trap (like illegal instruction)
    panic("Unhandled trap\n");
  }

	/*
  if ((sstatus & SSTATUS_SPP) == 0) {
    // printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n",
    // scause, sepc, r_stval());
    panic("Not from supervisor mode\n");
  }
  if (intr_get() != 0) {
    // printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n",
    // scause, sepc, r_stval());
    panic("Interrupts enabled\n");
  }
  if ((which_dev = dev_intr()) == 0) {
    int interrupt = (tf.cause >> 63);
    // printk("\n+++ Kernel Trap +++\nscause: %p\nsepc:   %p\nstval:  %p\n",
    // scause, sepc, r_stval());
    panic("Unhandled trap\n");
  }
	*/

	/*
  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2)
    ;
  // yield();
	*/

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}
