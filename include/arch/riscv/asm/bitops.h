#ifndef _ASM_RISCV_BITOPS_H
#define _ASM_RISCV_BITOPS_H

/*
 * Copyright 1992, Linus Torvalds.
 *
 * Note: inlines with more than a single statement should be marked
 * __always_inline to avoid problems with older gcc's inlining heuristics.
 */

#ifndef __BITOPS_H__
#error only <lib/bitops.h> can be included directly
#endif

#include <nautilus/intrinsics.h>


#define BIT_64(n)			(U64_C(1) << (n))

#define __AMO(op)	"amo" #op ".d"

// #define BIT_MASK(nr)        ((unsigned long) 1 << ((nr) % BITS_PER_LONG))

#define __test_and_op_bit_ord(op, mod, nr, addr, ord)		\
({								\
	unsigned long __res, __mask;				\
	__mask = BIT_MASK(nr);					\
	__asm__ __volatile__ (					\
		__AMO(op) #ord " %0, %2, %1"			\
		: "=r" (__res), "+A" (addr[BIT_WORD(nr)])	\
		: "r" (mod(__mask))				\
		: "memory");					\
	((__res & __mask) != 0);				\
})

#define __op_bit_ord(op, mod, nr, addr, ord)			\
	__asm__ __volatile__ (					\
		__AMO(op) #ord " zero, %1, %0"			\
		: "+A" (addr[BIT_WORD(nr)])			\
		: "r" (mod(BIT_MASK(nr)))			\
		: "memory");

#define __test_and_op_bit(op, mod, nr, addr) 			\
	__test_and_op_bit_ord(op, mod, nr, addr, .aqrl)
#define __op_bit(op, mod, nr, addr)				\
	__op_bit_ord(op, mod, nr, addr, )

/* Bitmask modifiers */
#define __NOP(x)	(x)
#define __NOT(x)	(~(x))


/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1)
/* Technically wrong, but this avoids compilation errors on some gcc
   versions. */
#define BITOP_ADDR(x) "=m" (*(volatile long *) (x))
#else
#define BITOP_ADDR(x) "+m" (*(volatile long *) (x))
#endif

#define ADDR				BITOP_ADDR(addr)

/*
 * We do the locked ops that don't return the old value as
 * a mask operation on a byte.
 */
#define IS_IMMEDIATE(nr)		(__builtin_constant_p(nr))
#define CONST_MASK_ADDR(nr, addr)	BITOP_ADDR((void *)(addr) + ((nr)>>3))
#define CONST_MASK(nr)			(1 << ((nr) & 7))


/**
 * __set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike set_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline void set_bit(int nr, volatile unsigned long *addr)
{
    __op_bit(or, __NOP, nr, addr);
}


static inline void clear_bit(int nr, volatile unsigned long *addr)
{
    __op_bit(and, __NOT, nr, addr);
}

static inline int test_bit(unsigned int nr, const volatile unsigned long *addr)
{
    return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static __inline__ int test_and_set_bit(int nr, volatile unsigned long * addr)
{
    return __test_and_op_bit(or, __NOP, nr, addr);
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static __inline__ int test_and_clear_bit(int nr, volatile unsigned long * addr)
{
    return __test_and_op_bit(and, __NOT, nr, addr);
}

/**
 * __change_bit - Toggle a bit in memory
 * @nr: the bit to change
 * @addr: the address to start counting from
 *
 * Unlike change_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline void change_bit(int nr, volatile unsigned long *addr)
{
        __test_and_op_bit(xor, __NOP, nr, addr);
}


/**
 * __ffs - find first set bit in word
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static inline unsigned long __ffs(unsigned long word)
{
        int num = 0;

	if ((word & 0xffffffff) == 0) {
		num += 32;
		word >>= 32;
	}
	if ((word & 0xffff) == 0) {
		num += 16;
		word >>= 16;
	}
	if ((word & 0xff) == 0) {
		num += 8;
		word >>= 8;
	}
	if ((word & 0xf) == 0) {
		num += 4;
		word >>= 4;
	}
	if ((word & 0x3) == 0) {
		num += 2;
		word >>= 2;
	}
	if ((word & 0x1) == 0)
		num += 1;
	return num;
}

/**
 * ffz - find first zero bit in word
 * @word: The word to search
 *
 * Undefined if no zero exists, so code should check against ~0UL first.
 */
static inline unsigned long ffz(unsigned long word)
{
	return __ffs(~word);
}

/*
 * __fls: find last set bit in word
 * @word: The word to search
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static inline unsigned long __fls(unsigned long word)
{
        int num = BITS_PER_LONG - 1;

	if (!(word & (~0ul << (BITS_PER_LONG-16)))) {
		num -= 16;
		word <<= 16;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-8)))) {
		num -= 8;
		word <<= 8;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-4)))) {
		num -= 4;
		word <<= 4;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-2)))) {
		num -= 2;
		word <<= 2;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-1))))
		num -= 1;
	return num;
}

#undef ADDR

/**
 * ffs - find first set bit in word
 * @x: the word to search
 *
 * This is defined the same way as the libc and compiler builtin ffs
 * routines, therefore differs in spirit from the other bitops.
 *
 * ffs(value) returns 0 if value is 0 or the position of the first
 * set bit if value is nonzero. The first (least significant) bit
 * is at position 1.
 */
static inline int ffs(int x)
{
	int r = 1;

	/*
	 * AMD64 says BSFL won't clobber the dest reg if x==0; Intel64 says the
	 * dest reg is undefined if x==0, but their CPU architect says its
	 * value is written to set it to the same as before, except that the
	 * top 32 bits will be cleared.
	 *
	 * We cannot do this on 32 bits because at the very least some
	 * 486 CPUs did not behave this way.
	 */
	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

/**
 * fls - find last set bit in word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffs, but returns the position of the most significant set bit.
 *
 * fls(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 32.
 */
static inline int fls(int x)
{
	int r;

	/*
	 * AMD64 says BSRL won't clobber the dest reg if x==0; Intel64 says the
	 * dest reg is undefined if x==0, but their CPU architect says its
	 * value is written to set it to the same as before, except that the
	 * top 32 bits will be cleared.
	 *
	 * We cannot do this on 32 bits because at the very least some
	 * 486 CPUs did not behave this way.
	 */
        int num = 32 - 1;

	if (!(x & (~0ul << (32-16)))) {
		num -= 16;
		x <<= 16;
	}
	if (!(x & (~0ul << (32-8)))) {
		num -= 8;
		x <<= 8;
	}
	if (!(x & (~0ul << (32-4)))) {
		num -= 4;
		x <<= 4;
	}
	if (!(x & (~0ul << (32-2)))) {
		num -= 2;
		x <<= 2;
	}
	if (!(x & (~0ul << (32-1))))
		num -= 1;
	return num;
}

/**
 * fls64 - find last set bit in a 64-bit word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffsll, but returns the position of the most significant set bit.
 *
 * fls64(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 64.
 */
static inline int fls64(uint64_t x)
{
	/*
	 * AMD64 says BSRQ won't clobber the dest reg if x==0; Intel64 says the
	 * dest reg is undefined if x==0, but their CPU architect says its
	 * value is written to set it to the same as before.
	 */
	int num = BITS_PER_LONG - 1;

	if (!(x & (~0ul << 32))) {
		num -= 32;
		x <<= 32;
	}
	if (!(x & (~0ul << (BITS_PER_LONG-16)))) {
		num -= 16;
		x <<= 16;
	}
	if (!(x & (~0ul << (BITS_PER_LONG-8)))) {
		num -= 8;
		x <<= 8;
	}
	if (!(x & (~0ul << (BITS_PER_LONG-4)))) {
		num -= 4;
		x <<= 4;
	}
	if (!(x & (~0ul << (BITS_PER_LONG-2)))) {
		num -= 2;
		x <<= 2;
	}
	if (!(x & (~0ul << (BITS_PER_LONG-1))))
		num -= 1;
	return num;
}

#endif /* _ASM_RISCV_BITOPS_H */
