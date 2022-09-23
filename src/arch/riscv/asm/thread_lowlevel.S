#include <asm/lowlevel.h>
#include <nautilus/gdt.h>
#include <nautilus/thread.h>

#define CTX_SIZE (31 * SZREG)

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

#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9)  // external
#define SIE_STIE (1L << 5)  // timer
#define SIE_SSIE (1L << 1)  // software

ENTRY(nk_thread_switch)

	# allocate space on the stack for the registers
	addi sp, sp, -CTX_SIZE
	# store the return address
	REG_S ra, ROFF(0, sp)
	# not sp
	REG_S gp, ROFF(2, sp)
	# not tp
	REG_S t0, ROFF(4, sp)
	REG_S t1, ROFF(5, sp)
	REG_S t2, ROFF(6, sp)
	REG_S s0, ROFF(7, sp)
	REG_S s1, ROFF(8, sp)
	REG_S a0, ROFF(9, sp)
	REG_S a1, ROFF(10,sp)
	REG_S a2, ROFF(11, sp)
	REG_S a3, ROFF(12, sp)
	REG_S a4, ROFF(13, sp)
	REG_S a5, ROFF(14, sp)
	REG_S a6, ROFF(15, sp)
	REG_S a7, ROFF(16, sp)
	REG_S s2, ROFF(17, sp)
	REG_S s3, ROFF(18, sp)
	REG_S s4, ROFF(19, sp)
	REG_S s5, ROFF(20, sp)
	REG_S s6, ROFF(21, sp)
	REG_S s7, ROFF(22, sp)
	REG_S s8, ROFF(23, sp)
	REG_S s9, ROFF(24, sp)
	REG_S s10, ROFF(25, sp)
	REG_S s11, ROFF(26, sp)
	REG_S t3, ROFF(27, sp)
	REG_S t4, ROFF(28, sp)
	REG_S t5, ROFF(29, sp)
	REG_S t6, ROFF(30, sp)

	lwu t0, 8(tp)               # increment interrupt nesting level
	addi t0, t0, 1              # pretend we are doing an interrupt-driven switch
	sw  t0, 8(tp)               # to make the sums work out

ENTRY(nk_thread_switch_intr_entry)

	# load cur_thread from the cpu structure
	REG_L t0,  ROFF(0,  tp)
	# store the stack pointer into it
	REG_S sp,  ROFF(0,  t0)

# On a thread exit we must avoid saving thread state
# or using the stack at all since at this point the thread and its stack
# might already be deleted.  Interrupts are off, however.
# We also want to save the cost of any state saves
# We should get here via a JUMP
# Note that the first three instructions establish the new stack
# and thread structure
ENTRY(nk_thread_switch_on_exit)

	# set the CPUs cur_thread to the next thread (argument to this function)
	REG_S a0,  ROFF(0, tp)
	# load the stack pointer from the current (new) thread
	REG_L sp,  ROFF(0, a0)

	lwu t0, 8(tp)                // decrement interrupt nesting level
	addi t0, t0, -1              // we may now be leaving interrupt context
	sw  t0, 8(tp)

	# restore registers
	REG_L ra, ROFF(0, sp)
	REG_L gp, ROFF(2, sp)
	# REG_L tp, ROFF(3, sp)
	# REG_L t0, ROFF(4, sp)
	# REG_L t1, ROFF(5, sp)
	REG_L t2, ROFF(6, sp)
	REG_L s0, ROFF(7, sp)
	REG_L s1, ROFF(8, sp)
	REG_L a0, ROFF(9, sp)
	REG_L a1, ROFF(10,sp)
	REG_L a2, ROFF(11, sp)
	REG_L a3, ROFF(12, sp)
	REG_L a4, ROFF(13, sp)
	REG_L a5, ROFF(14, sp)
	REG_L a6, ROFF(15, sp)
	REG_L a7, ROFF(16, sp)
	REG_L s2, ROFF(17, sp)
	REG_L s3, ROFF(18, sp)
	REG_L s4, ROFF(19, sp)
	REG_L s5, ROFF(20, sp)
	REG_L s6, ROFF(21, sp)
	REG_L s7, ROFF(22, sp)
	REG_L s8, ROFF(23, sp)
	REG_L s9, ROFF(24, sp)
	REG_L s10, ROFF(25, sp)
	REG_L s11, ROFF(26, sp)
	REG_L t3, ROFF(27, sp)
	REG_L t4, ROFF(28, sp)
	REG_L t5, ROFF(29, sp)
	REG_L t6, ROFF(30, sp)

    // ensure enable supervisor interrupts
    li      t0, SIE_SEIE
	or      t0, t0, SIE_STIE
	or      t0, t0, SIE_SSIE
    csrs    sie, t0

	csrr    t0, sstatus
	li      t1, SSTATUS_SIE
	or      t0, t0, t1
	csrw    sstatus, t0

	// restore temp registers
	REG_L t0, ROFF(4, sp)
	REG_L t1, ROFF(5, sp)
	addi sp, sp, CTX_SIZE

	ret                          // and off we go

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
	sw a2, 0(a1)

	// At this point, we are racing with the reaper on a different core,
	// which might take away our stack.  However, we have interrupts
	// off locally, so as long as *we* don't use the stack, we should be OK
	j nk_thread_switch_on_exit

	// and this should never happen...
	ret



ENTRY(nk_thread_entry)

	// get the function from the stack, store in ra
	REG_L ra, ROFF(1, sp)

	addi sp, sp, 2*SZREG

	jalr ra

	li a0, 0
	call nk_thread_exit
