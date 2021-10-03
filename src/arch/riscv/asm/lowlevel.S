# lowlevel.S - All low level functionality (boot and util)
# riscv64 bootloader for ChariotOS
# Nick Wanninger
# 29 December, 2020
#include <asm/lowlevel.h>

/* Macros for pointer/register sizes */
#define PTRLOG 3
#define SZREG 8
#define REG_S sd
#define REG_L ld
#define REG_SC sc.d
#define ROFF(N, R) N* SZREG(R)

/* Status register flags */
#define SR_SIE		(0x00000002UL) /* Supervisor Interrupt Enable */
#define SR_MIE		(0x00000008UL) /* Machine Interrupt Enable */
#define SR_SPIE		(0x00000020UL) /* Previous Supervisor IE */
#define SR_MPIE		(0x00000080UL) /* Previous Machine IE */
#define SR_SPP		(0x00000100UL) /* Previously Supervisor */
#define SR_MPP		(0x00001800UL) /* Previously Machine */
#define SR_SUM		(0x00040000UL) /* Supervisor User Memory Access */

#define CSR_SSTATUS		0x100
#define CSR_SIE			0x104
#define CSR_STVEC		0x105
#define CSR_SCOUNTEREN		0x106
#define CSR_SSCRATCH		0x140
#define CSR_SEPC		0x141
#define CSR_SCAUSE		0x142
#define CSR_STVAL		0x143
#define CSR_SIP			0x144
#define CSR_SATP		0x180

#define MSTATUS_MPP_MASK (3L << 11)  // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)  // machine-mode interrupt enable.

#define SIE_SEIE (1L << 9)  // external
#define SIE_STIE (1L << 5)  // timer
#define SIE_SSIE (1L << 1)  // software

#define MIE_MEIE (1L << 11)  // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software


# .option norvc # Disable instruction compression
.section .boot

GLOBAL(nautilus_entry)

    # OpenSBI passes us the hartid through a0.

	# starting stack at boot_stack_start
	la      sp, boot_stack_start
    li      t0, 2 * 4096 # 2 page stack
    addi    t1, a0, 1 # t1 = mhartid + 1
    mul     t0, t0, t1 # t1 = SIZE * (mhartid + 1)
    add     sp, sp, t0 # sp = boot_stack_start + (SIZE * (mhartid + 1))

	# zero out bss
	lla     t0, _bssStart
	lla     t1, _bssEnd
	beq     t0, t1, 1f
0:
	REG_S   zero, (t0)
	add     t0, t0, SZREG
	blt     t1, t0, 0b  # if t1 is less than t0
1:
	# allocate space for the scratch pad
	addi    sp, sp, -32 * SZREG

	# disable supervisor address translation and protection (paging), since we'd identity map anyways
    csrw    satp, 0
    # flush tlb and whatnot
    sfence.vma zero, zero

    # enable supervisor interrupts
	li      t0, SIE_SEIE | SIE_STIE | SIE_SSIE
	csrs    sie, t0

    la      t0, main_ptr
    REG_L   t0, 0(t0)

	jr      t0

.section .text

.balign 8
main_ptr: .quad main

# When the processor gets an interrupt or traps, it jumps here and sets some CSRs
# like sstatus, sbadaddr, stval etc... The only problem is that we don't switch
# stacks when jumping from userspace. To fix this, we gotta do some extra nonsense
# to avoid losing register states
#define TP_BAK1 0
#define TP_BAK2 1
#define TP_BAK3 2
#define TP_TCA  3
#define TP_INTERVAL  4
#define TP_KERNEL_SP 5
#define TP_USER_SP 6
#define TF_SIZE_ON_STACK (36 * SZREG)

GLOBAL(kernel_vec)

        // make room to save registers.
        addi sp, sp, -256

        // save the registers.
        sd ra, 0(sp)
        sd sp, 8(sp)
        sd gp, 16(sp)
        sd tp, 24(sp)
        sd t0, 32(sp)
        sd t1, 40(sp)
        sd t2, 48(sp)
        sd s0, 56(sp)
        sd s1, 64(sp)
        sd a0, 72(sp)
        sd a1, 80(sp)
        sd a2, 88(sp)
        sd a3, 96(sp)
        sd a4, 104(sp)
        sd a5, 112(sp)
        sd a6, 120(sp)
        sd a7, 128(sp)
        sd s2, 136(sp)
        sd s3, 144(sp)
        sd s4, 152(sp)
        sd s5, 160(sp)
        sd s6, 168(sp)
        sd s7, 176(sp)
        sd s8, 184(sp)
        sd s9, 192(sp)
        sd s10, 200(sp)
        sd s11, 208(sp)
        sd t3, 216(sp)
        sd t4, 224(sp)
        sd t5, 232(sp)
        sd t6, 240(sp)

	// call the C trap handler in idt.c
        call kernel_trap

        // restore registers.
        ld ra, 0(sp)
        ld sp, 8(sp)
        ld gp, 16(sp)
        // not this, in case we moved CPUs: ld tp, 24(sp)
        ld t0, 32(sp)
        ld t1, 40(sp)
        ld t2, 48(sp)
        ld s0, 56(sp)
        ld s1, 64(sp)
        ld a0, 72(sp)
        ld a1, 80(sp)
        ld a2, 88(sp)
        ld a3, 96(sp)
        ld a4, 104(sp)
        ld a5, 112(sp)
        ld a6, 120(sp)
        ld a7, 128(sp)
        ld s2, 136(sp)
        ld s3, 144(sp)
        ld s4, 152(sp)
        ld s5, 160(sp)
        ld s6, 168(sp)
        ld s7, 176(sp)
        ld s8, 184(sp)
        ld s9, 192(sp)
        ld s10, 200(sp)
        ld s11, 208(sp)
        ld t3, 216(sp)
        ld t4, 224(sp)
        ld t5, 232(sp)
        ld t6, 240(sp)

        addi sp, sp, 256

        // return to whatever we were doing in the kernel.
        sret

END(kernel_vec)

# TODO: 32 vs 64bit :)
ENTRY(context_switch)
#define CTX_SIZE (13 * SZREG)

	# a0: address to place stack pointer
	# a1: new stack pointer

	addi sp, sp, -CTX_SIZE # 128 is a round up, as we only need 104. TODO:

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

	REG_S sp, (a0)
	mv sp, a1

	REG_L ra,  ROFF(0,  sp)
	REG_L s0,  ROFF(1,  sp)
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

	ret
END(context_switch)

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

ENTRY(__rv_save_fpu)
	fsd f0,  8 * 0(a0)
	fsd f1,  8 * 1(a0)
	fsd f2,  8 * 2(a0)
	fsd f3,  8 * 3(a0)
	fsd f4,  8 * 4(a0)
	fsd f5,  8 * 5(a0)
	fsd f6,  8 * 6(a0)
	fsd f7,  8 * 7(a0)
	fsd f8,  8 * 8(a0)
	fsd f9,  8 * 9(a0)
	fsd f10, 8 * 10(a0)
	fsd f11, 8 * 11(a0)
	fsd f12, 8 * 12(a0)
	fsd f13, 8 * 13(a0)
	fsd f14, 8 * 14(a0)
	fsd f15, 8 * 15(a0)
	fsd f16, 8 * 16(a0)
	fsd f17, 8 * 17(a0)
	fsd f18, 8 * 18(a0)
	fsd f19, 8 * 19(a0)
	fsd f20, 8 * 20(a0)
	fsd f21, 8 * 21(a0)
	fsd f22, 8 * 22(a0)
	fsd f23, 8 * 23(a0)
	fsd f24, 8 * 24(a0)
	fsd f25, 8 * 25(a0)
	fsd f26, 8 * 26(a0)
	fsd f27, 8 * 27(a0)
	fsd f28, 8 * 28(a0)
	fsd f29, 8 * 29(a0)
	fsd f30, 8 * 30(a0)
	fsd f31, 8 * 31(a0)
	ret
END(__rv_save_fpu)

ENTRY(__rv_load_fpu)
	fld f0,  8 * 0(a0)
	fld f1,  8 * 1(a0)
	fld f2,  8 * 2(a0)
	fld f3,  8 * 3(a0)
	fld f4,  8 * 4(a0)
	fld f5,  8 * 5(a0)
	fld f6,  8 * 6(a0)
	fld f7,  8 * 7(a0)
	fld f8,  8 * 8(a0)
	fld f9,  8 * 9(a0)
	fld f10, 8 * 10(a0)
	fld f11, 8 * 11(a0)
	fld f12, 8 * 12(a0)
	fld f13, 8 * 13(a0)
	fld f14, 8 * 14(a0)
	fld f15, 8 * 15(a0)
	fld f16, 8 * 16(a0)
	fld f17, 8 * 17(a0)
	fld f18, 8 * 18(a0)
	fld f19, 8 * 19(a0)
	fld f20, 8 * 20(a0)
	fld f21, 8 * 21(a0)
	fld f22, 8 * 22(a0)
	fld f23, 8 * 23(a0)
	fld f24, 8 * 24(a0)
	fld f25, 8 * 25(a0)
	fld f26, 8 * 26(a0)
	fld f27, 8 * 27(a0)
	fld f28, 8 * 28(a0)
	fld f29, 8 * 29(a0)
	fld f30, 8 * 30(a0)
	fld f31, 8 * 31(a0)
	ret
END(__rv_load_fpu)

/*
	rax = nk_lowlevel_memset(rdi=dest, rsi=src, rdx=count)
*/
/* void *memset(void *, int, size_t) */
.global nk_low_level_memset
nk_low_level_memset:
	move t0, a0  /* Preserve return value */

	/* Defer to byte-oriented fill for small sizes */
	sltiu a3, a2, 16
	bnez a3, 4f

	/*
	 * Round to nearest XLEN-aligned address
	 * greater than or equal to start address
	 */
	addi a3, t0, SZREG-1
	andi a3, a3, ~(SZREG-1)
	beq a3, t0, 2f  /* Skip if already aligned */
	/* Handle initial misalignment */
	sub a4, a3, t0
1:
	sb a1, 0(t0)
	addi t0, t0, 1
	bltu t0, a3, 1b
	sub a2, a2, a4  /* Update count */

2: /* Duff's device with 32 XLEN stores per iteration */
	/* Broadcast value into all bytes */
	andi a1, a1, 0xff
	slli a3, a1, 8
	or a1, a3, a1
	slli a3, a1, 16
	or a1, a3, a1
	slli a3, a1, 32
	or a1, a3, a1

	/* Calculate end address */
	andi a4, a2, ~(SZREG-1)
	add a3, t0, a4

	andi a4, a4, 31*SZREG  /* Calculate remainder */
	beqz a4, 3f            /* Shortcut if no remainder */
	neg a4, a4
	addi a4, a4, 32*SZREG  /* Calculate initial offset */

	/* Adjust start address with offset */
	sub t0, t0, a4

	/* Jump into loop body */
	/* Assumes 32-bit instruction lengths */
	la a5, 3f
	srli a4, a4, 1
	add a5, a5, a4
	jr a5
3:
	REG_S a1,        0(t0)
	REG_S a1,    SZREG(t0)
	REG_S a1,  2*SZREG(t0)
	REG_S a1,  3*SZREG(t0)
	REG_S a1,  4*SZREG(t0)
	REG_S a1,  5*SZREG(t0)
	REG_S a1,  6*SZREG(t0)
	REG_S a1,  7*SZREG(t0)
	REG_S a1,  8*SZREG(t0)
	REG_S a1,  9*SZREG(t0)
	REG_S a1, 10*SZREG(t0)
	REG_S a1, 11*SZREG(t0)
	REG_S a1, 12*SZREG(t0)
	REG_S a1, 13*SZREG(t0)
	REG_S a1, 14*SZREG(t0)
	REG_S a1, 15*SZREG(t0)
	REG_S a1, 16*SZREG(t0)
	REG_S a1, 17*SZREG(t0)
	REG_S a1, 18*SZREG(t0)
	REG_S a1, 19*SZREG(t0)
	REG_S a1, 20*SZREG(t0)
	REG_S a1, 21*SZREG(t0)
	REG_S a1, 22*SZREG(t0)
	REG_S a1, 23*SZREG(t0)
	REG_S a1, 24*SZREG(t0)
	REG_S a1, 25*SZREG(t0)
	REG_S a1, 26*SZREG(t0)
	REG_S a1, 27*SZREG(t0)
	REG_S a1, 28*SZREG(t0)
	REG_S a1, 29*SZREG(t0)
	REG_S a1, 30*SZREG(t0)
	REG_S a1, 31*SZREG(t0)
	addi t0, t0, 32*SZREG
	bltu t0, a3, 3b
	andi a2, a2, SZREG-1  /* Update count */

4:
	/* Handle trailing misalignment */
	beqz a2, 6f
	add a3, t0, a2
5:
	sb a1, 0(t0)
	addi t0, t0, 1
	bltu t0, a3, 5b
6:
	ret

/*
	rax = nk_low_level_memcpy(rdi=dest, rsi=src, rdx=count)
*/
/* void *memcpy(void *, const void *, size_t) */
.global nk_low_level_memcpy
nk_low_level_memcpy:
	move t6, a0  /* Preserve return value */

	/* Defer to byte-oriented copy for small sizes */
	sltiu a3, a2, 128
	bnez a3, 4f
	/* Use word-oriented copy only if low-order bits match */
	andi a3, t6, SZREG-1
	andi a4, a1, SZREG-1
	bne a3, a4, 4f

	beqz a3, 2f  /* Skip if already aligned */
	/*
	 * Round to nearest double word-aligned address
	 * greater than or equal to start address
	 */
	andi a3, a1, ~(SZREG-1)
	addi a3, a3, SZREG
	/* Handle initial misalignment */
	sub a4, a3, a1
1:
	lb a5, 0(a1)
	addi a1, a1, 1
	sb a5, 0(t6)
	addi t6, t6, 1
	bltu a1, a3, 1b
	sub a2, a2, a4  /* Update count */

2:
	andi a4, a2, ~((16*SZREG)-1)
	beqz a4, 4f
	add a3, a1, a4
3:
	REG_L a4,       0(a1)
	REG_L a5,   SZREG(a1)
	REG_L a6, 2*SZREG(a1)
	REG_L a7, 3*SZREG(a1)
	REG_L t0, 4*SZREG(a1)
	REG_L t1, 5*SZREG(a1)
	REG_L t2, 6*SZREG(a1)
	REG_L t3, 7*SZREG(a1)
	REG_L t4, 8*SZREG(a1)
	REG_L t5, 9*SZREG(a1)
	REG_S a4,       0(t6)
	REG_S a5,   SZREG(t6)
	REG_S a6, 2*SZREG(t6)
	REG_S a7, 3*SZREG(t6)
	REG_S t0, 4*SZREG(t6)
	REG_S t1, 5*SZREG(t6)
	REG_S t2, 6*SZREG(t6)
	REG_S t3, 7*SZREG(t6)
	REG_S t4, 8*SZREG(t6)
	REG_S t5, 9*SZREG(t6)
	REG_L a4, 10*SZREG(a1)
	REG_L a5, 11*SZREG(a1)
	REG_L a6, 12*SZREG(a1)
	REG_L a7, 13*SZREG(a1)
	REG_L t0, 14*SZREG(a1)
	REG_L t1, 15*SZREG(a1)
	addi a1, a1, 16*SZREG
	REG_S a4, 10*SZREG(t6)
	REG_S a5, 11*SZREG(t6)
	REG_S a6, 12*SZREG(t6)
	REG_S a7, 13*SZREG(t6)
	REG_S t0, 14*SZREG(t6)
	REG_S t1, 15*SZREG(t6)
	addi t6, t6, 16*SZREG
	bltu a1, a3, 3b
	andi a2, a2, (16*SZREG)-1  /* Update count */

4:
	/* Handle trailing misalignment */
	beqz a2, 6f
	add a3, a1, a2

	/* Use word-oriented copy if co-aligned to word boundary */
	or a5, a1, t6
	or a5, a5, a3
	andi a5, a5, 3
	bnez a5, 5f
7:
	lw a4, 0(a1)
	addi a1, a1, 4
	sw a4, 0(t6)
	addi t6, t6, 4
	bltu a1, a3, 7b

	ret

5:
	lb a4, 0(a1)
	addi a1, a1, 1
	sb a4, 0(t6)
	addi t6, t6, 1
	bltu a1, a3, 5b
6:
	ret
