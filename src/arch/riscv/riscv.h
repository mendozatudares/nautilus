/* Macros for pointer/register sizes */
#define PTRLOG 3
#define SZREG 8
#define REG_S sd
#define REG_L ld
#define REG_SC sc.d
#define ROFF(N, R) N* SZREG(R)


/* Macros for paging */
#define PGSIZE 4096  // bytes per page
#define PGSHIFT 12   // bits of offset within a page

#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

/*
 * VM_BITS      - the overall bits (typically XX in SVXX)
 * VM_PART_BITS - how many bits per VPN[x]
 * VM_PART_NUM  - how many parts are there?
 */

#define VM_NORMAL_PAGE (0x1000L)
#define VM_MEGA_PAGE (VM_NORMAL_PAGE << 9)
#define VM_GIGA_PAGE (VM_MEGA_PAGE << 9)
#define VM_TERA_PAGE (VM_GIGA_PAGE << 9)

/* RISC-V Sv48 - 48 bit virtual memory on 64bit */
#define VM_PART_BITS 9
#define VM_PART_NUM 4

#define VM_BIGGEST_PAGE (VM_NORMAL_PAGE << ((VM_PART_NUM - 1) * VM_PART_BITS))

#define VM_BITS ((VM_PART_BITS * VM_PART_NUM) + PGSHIFT)

#define PT_V (1 << 0)
#define PT_R (1 << 1)
#define PT_W (1 << 2)
#define PT_X (1 << 3)
#define PT_U (1 << 4)
#define PT_G (1 << 5)
#define PT_A (1 << 6)
#define PT_D (1 << 7)

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64_t)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)

#define PTE_FLAGS(pte) ((pte)&0x3FF)

#define MAKE_PTE(pa, flags) (PA2PTE(pa) | (flags))

// extract the three N-bit page table indices from a virtual address.
#define PXMASK ((1llu << VM_PART_BITS) - 1)  // N bits
#define PXSHIFT(level) (PGSHIFT + (VM_PART_BITS * (level)))
#define PX(level, va) ((((uint64_t)(va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// SvXX, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << ((VM_PART_BITS * VM_PART_NUM) + 12 - 1))

#define SATP_FLAG 9
#define SATP_SHIFT 60
#define SATP_MODE (9L << 60)
#define MAKE_SATP(page_table) (SATP_MODE | (((uint64_t)page_table) >> 12))


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

#define MIE_MEIE (1L << 11)  // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

#define SIE_SEIE (1L << 9)  // external
#define SIE_STIE (1L << 5)  // timer
#define SIE_SSIE (1L << 1)  // software


// which hart (core) is this?
static inline uint64_t r_mhartid()
{
  uint64_t x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

static inline uint64_t r_mstatus()
{
  uint64_t x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void w_mstatus(uint64_t x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_mepc(uint64_t x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

static inline uint64_t r_sstatus()
{
  uint64_t x;
  asm volatile("csrr %0, sstatus" : "=r" (x) );
  return x;
}

static inline void w_sstatus(uint64_t x)
{
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

// Supervisor Interrupt Pending
static inline uint64_t r_sip()
{
  uint64_t x;
  asm volatile("csrr %0, sip" : "=r" (x) );
  return x;
}

static inline void w_sip(uint64_t x)
{
  asm volatile("csrw sip, %0" : : "r" (x));
}

static inline uint64_t r_sie()
{
  uint64_t x;
  asm volatile("csrr %0, sie" : "=r" (x) );
  return x;
}

static inline void w_sie(uint64_t x)
{
  asm volatile("csrw sie, %0" : : "r" (x));
}

static inline uint64_t r_mie()
{
  uint64_t x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

static inline void w_mie(uint64_t x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_sepc(uint64_t x)
{
  asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline uint64_t r_sepc()
{
  uint64_t x;
  asm volatile("csrr %0, sepc" : "=r" (x) );
  return x;
}

// Machine Exception Delegation
static inline uint64_t r_medeleg()
{
  uint64_t x;
  asm volatile("csrr %0, medeleg" : "=r" (x) );
  return x;
}

static inline void w_medeleg(uint64_t x)
{
  asm volatile("csrw medeleg, %0" : : "r" (x));
}

// Machine Interrupt Delegation
static inline uint64_t r_mideleg()
{
  uint64_t x;
  asm volatile("csrr %0, mideleg" : "=r" (x) );
  return x;
}

static inline void w_mideleg(uint64_t x)
{
  asm volatile("csrw mideleg, %0" : : "r" (x));
}

// Supervisor Trap-Vector Base Address
// low two bits are mode.
static inline void w_stvec(uint64_t x)
{
  asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64_t r_stvec()
{
  uint64_t x;
  asm volatile("csrr %0, stvec" : "=r" (x) );
  return x;
}

// Machine-mode interrupt vector
static inline void w_mtvec(uint64_t x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

// supervisor address translation and protection;
// holds the address of the page table.
static inline void w_satp(uint64_t x)
{
  asm volatile("csrw satp, %0" : : "r" (x));
}

static inline uint64_t r_satp()
{
  uint64_t x;
  asm volatile("csrr %0, satp" : "=r" (x) );
  return x;
}

// Supervisor Scratch register, for early trap handler in trampoline.S.
static inline void w_sscratch(uint64_t x)
{
  asm volatile("csrw sscratch, %0" : : "r" (x));
}

static inline void w_mscratch(uint64_t x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

// Supervisor Trap Cause
static inline uint64_t r_scause()
{
  uint64_t x;
  asm volatile("csrr %0, scause" : "=r" (x) );
  return x;
}

// Supervisor Trap Value
static inline uint64_t r_stval()
{
  uint64_t x;
  asm volatile("csrr %0, stval" : "=r" (x) );
  return x;
}

// Machine-mode Counter-Enable
static inline void w_mcounteren(uint64_t x)
{
  asm volatile("csrw mcounteren, %0" : : "r" (x));
}

static inline uint64_t r_mcounteren()
{
  uint64_t x;
  asm volatile("csrr %0, mcounteren" : "=r" (x) );
  return x;
}

// machine-mode cycle counter
static inline uint64_t r_time()
{
  uint64_t x;
  asm volatile("csrr %0, time" : "=r" (x) );
  return x;
}

// enable device interrupts
static inline void intr_on()
{
  w_sstatus(r_sstatus() | SSTATUS_SIE);
}

// disable device interrupts
static inline void intr_off()
{
  w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

// are device interrupts enabled?
static inline int intr_get()
{
  uint64_t x = r_sstatus();
  return (x & SSTATUS_SIE) != 0;
}

static inline uint64_t r_sp()
{
  uint64_t x;
  asm volatile("mv %0, sp" : "=r" (x) );
  return x;
}

// read and write tp, the thread pointer, which holds
// this core's hartid (core number), the index into cpus[].
static inline uint64_t r_tp()
{
  uint64_t x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

static inline void w_tp(uint64_t x)
{
  asm volatile("mv tp, %0" : : "r" (x));
}

static inline uint64_t r_ra()
{
  uint64_t x;
  asm volatile("mv %0, ra" : "=r" (x) );
  return x;
}

// flush the TLB.
static inline void sfence_vma()
{
  // the zero, zero means flush all TLB entries.
  asm volatile("sfence.vma zero, zero");
}

typedef uint64_t pte_t;
typedef uint64_t *pagetable_t; // 512 PTEs
