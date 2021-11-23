#include <asm/lowlevel.h>
#include <nautilus/gdt.h>
#include <nautilus/thread.h>

#define PTRLOG 3
#define SZREG 8
#define REG_S sd
#define REG_L ld
#define REG_SC sc.d
#define ROFF(N, R) N * SZREG(R)

#define CTX_SIZE (13 * SZREG)

/*
 * We come in like this:
 * ----------
 * thread_ptr
 * ----------
 * ret addr
 * ----------
 *
 */
.section .text

ENTRY(nk_thread_switch)
    REG_L t0,  ROFF(0,  sp)     // load ret addr
    REG_S t0,  ROFF(-4, sp)     // move ret addr down (pass intr state holder)

    REG_S sp,  ROFF(0, sp)      // save sp

    addi sp, sp, -SZREG
    li t1, KERNEL_SS
    REG_S t1,  ROFF(0, sp)      // save SS

    addi sp, sp, -SZREG
    csrr t1, sstatus
    REG_S t1,  ROFF(0, sp)      // save sstatus

    addi sp, sp, -SZREG
    li t1, KERNEL_CS
    REG_S t1, ROFF(0, sp)       // push CS seg sel

    addi sp, sp, -2*SZREG
    REG_S zero, ROFF(0, sp)     // push fake intr number

    addi sp, sp, -CTX_SIZE      // SAVE_GRPS()
    REG_S ra,  ROFF(0,  sp)
    REG_S s0,  ROFF(1,  sp)
    REG_S s1,  ROFF(2,  sp)
    REG_S s2,  ROFF(3,  sp)
    REG_S s3,  ROFF(4,  sp)
    REG_S s4,  ROFF(5,  sp)
    REG_S s5,  ROFF(6,  sp)
    REG_S s6,  ROFF(7,  sp)
    REG_S s7,  ROFF(8,  sp)
    REG_S s8,  ROFF(9,  sp)
    REG_S s9,  ROFF(10, sp)
    REG_S s10, ROFF(11, sp)
    REG_S s11, ROFF(12, sp)

    lwu t0, 8(tp)               // increment interrupt nesting level
    addi t0, t0, 1              // pretend we are doing an interrupt-driven switch
    sw  t0, 8(tp)               // to make the sums work out

ENTRY(nk_thread_switch_intr_entry)
    REG_L t0,  ROFF(0,  tp)
    REG_S sp,  ROFF(0,  t0)     // save the current stack pointer

// On a thread exit we must avoid saving thread state
// or using the stack at all since at this point the thread and its stack
// might already be deleted.  Interrupts are off, however.
// We also want to save the cost of any state saves
// We should get here via a JUMP
// Note that the first three instructions establish the new stack
// and thread structure
ENTRY(nk_thread_switch_on_exit)
    REG_S a0,  ROFF(0, tp)       // make the thread pointer the current thread
    REG_L sp,  ROFF(0, a0)       // load its stack pointer

    REG_L ra,  ROFF(0,  sp)      // RESTORE_GPRs()
    REG_L s0,  ROFF(1,  sp)      // load the new thread's GRPs
    REG_L s1,  ROFF(2,  sp)
    REG_L s2,  ROFF(3,  sp)
    REG_L s3,  ROFF(4,  sp)
    REG_L s4,  ROFF(5,  sp)
    REG_L s5,  ROFF(6,  sp)
    REG_L s6,  ROFF(7,  sp)
    REG_L s7,  ROFF(8,  sp)
    REG_L s8,  ROFF(9,  sp)
    REG_L s9,  ROFF(10, sp)
    REG_L s10, ROFF(11, sp)
    REG_L s11, ROFF(12, sp)
    addi sp, sp, CTX_SIZE

    addi sp, sp, 2*SZREG        // pop off the vector and the error code

    lwu t0, 8(tp)               // decrement interrupt nesting level
    addi t0, t0, -1             // we may now be leaving interrupt context
    sw  t0, 8(tp)

    ret                         /* and off we go */

// rdi = new thread
// rsi = target address to write
// edx = value to write (as 4 bytes)
ENTRY(nk_thread_switch_exit_helper)
	// reset interrupt nesting
	// the context switch will decrement it to zero
    li t0, 1
    sw t0, 8(tp)

	// reset preemption disable level
    li t0, 0
    sw t0, 12(tp)

	// note that interrupts are still off

	// We now mark ourselves as toast for the reaper
    sw a2, ROFF(0,  a1)

	// At this point, we are racing with the reaper on a different core,
	// which might take away our stack.  However, we have interrupts
	// off locally, so as long as *we* don't use the stack, we should be OK
	j nk_thread_switch_on_exit

	// and this should never happen...
	ret



#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable

ENTRY(nk_thread_entry)

    // enable interrupts
    csrr    t0, sstatus
    li      t1, SSTATUS_SIE
    or      t0, t0, t1
    csrw    sstatus, t0

    // return
    ret

/* _setjmp functions */
ENTRY (_setjmp)
  li	a1, 0
  j	__sigsetjmp
END (_setjmp)
ENTRY (setjmp)
  li	a1, 1
  /* Fallthrough */
END (setjmp)
ENTRY (__sigsetjmp)
    REG_S ra,  0*SZREG(a0)
    REG_S s0,  1*SZREG(a0)
    REG_S s1,  2*SZREG(a0)
    REG_S s2,  3*SZREG(a0)
    REG_S s3,  4*SZREG(a0)
    REG_S s4,  5*SZREG(a0)
    REG_S s5,  6*SZREG(a0)
    REG_S s6,  7*SZREG(a0)
    REG_S s7,  8*SZREG(a0)
    REG_S s8,  9*SZREG(a0)
    REG_S s9, 10*SZREG(a0)
    REG_S s10,11*SZREG(a0)
    REG_S s11,12*SZREG(a0)
    REG_S sp, 13*SZREG(a0)

/* TODO: save signal mask? */
    li a0, 0
    ret
END (__sigsetjmp)

ENTRY (longjmp)
    REG_L ra,  0*SZREG(a0)
    REG_L s0,  1*SZREG(a0)
    REG_L s1,  2*SZREG(a0)
    REG_L s2,  3*SZREG(a0)
    REG_L s3,  4*SZREG(a0)
    REG_L s4,  5*SZREG(a0)
    REG_L s5,  6*SZREG(a0)
    REG_L s6,  7*SZREG(a0)
    REG_L s7,  8*SZREG(a0)
    REG_L s8,  9*SZREG(a0)
    REG_L s9, 10*SZREG(a0)
    REG_L s10,11*SZREG(a0)
    REG_L s11,12*SZREG(a0)
    REG_L sp, 13*SZREG(a0)

    seqz a0, a1
    add  a0, a0, a1   # a0 = (a1 == 0) ? 1 : a1
    ret
END (longjmp)
