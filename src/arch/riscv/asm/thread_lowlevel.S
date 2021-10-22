#include <asm/lowlevel.h>
#include <nautilus/gdt.h>
#include <nautilus/thread.h>

/* Macros for pointer/register sizes */
#define PTRLOG 3
#define SZREG 8
#define REG_S sd
#define REG_L ld
#define REG_SC sc.d
#define ROFF(N, R) N* SZREG(R)


ENTRY(context_switch)

#define CTX_SIZE (13 * SZREG)

    # a0: address to place stack pointer
    # a1: new stack pointer

    addi sp, sp, -CTX_SIZE # 128 is a round up, as we only need 104.

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