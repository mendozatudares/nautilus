# lowlevel.S - All low level functionality (boot and util)
# riscv64 bootloader for ChariotOS
# Nick Wanninger
# 29 December, 2020

#include <asm/lowlevel.h>

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

#define MSTATUS_MPP_MASK (3L << 11)  # previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)  # machine-mode interrupt enable.

#define SIE_SEIE (1L << 9)  # external
#define SIE_STIE (1L << 5)  # timer
#define SIE_SSIE (1L << 1)  # software

#define MIE_MEIE (1L << 11)  # external
#define MIE_MTIE (1L << 7)  # timer
#define MIE_MSIE (1L << 3)  # software


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
    li      t0, SIE_SEIE
    or      t0, t0, SIE_STIE
    or      t0, t0, SIE_SSIE
    csrs    sie, t0

    la      t0, main_ptr
    REG_L   t0, 0(t0)

    jr      t0


.balign 8
main_ptr: .quad main

.global secondary_core_stack
secondary_core_stack:
    .quad 0

.section .text

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

.balign 4
GLOBAL(kernel_vec)

    # make room to save registers.
    addi sp, sp, -288 # 38 registers

    # save the registers.
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

    # call the C trap handler in idt.c
		move a0, sp /* arg0 <- trapframe */
    call kernel_trap

    # restore registers.
    ld ra, 0(sp)
    # ld sp, 8(sp)
    ld gp, 16(sp)
    # not this, in case we moved CPUs: ld tp, 24(sp)
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

    addi sp, sp, 288

    # return to whatever we were doing in the kernel.
    sret

END(kernel_vec)

ENTRY(nk_fp_save)
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
END(nk_fp_save)

ENTRY(nk_fp_restore)
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
END(nk_fp_restore)
