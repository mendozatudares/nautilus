#include <asm/lowlevel.h>

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
