#define RISCV_CLOCKS_PER_SECOND 1000000
#define TICK_INTERVAL (RISCV_CLOCKS_PER_SECOND / NAUT_CONFIG_HZ)

#define read_csr(name)                         \
  ({                                           \
    uint64_t x;                             \
    asm volatile("csrr %0, " #name : "=r"(x)); \
    x;                                         \
  })

#define write_csr(csr, val)                                             \
  ({                                                                    \
    unsigned long __v = (unsigned long)(val);                           \
    __asm__ __volatile__("csrw " #csr ", %0" : : "rK"(__v) : "memory"); \
  })


#define set_csr(csr, val)                                               \
  ({                                                                    \
    unsigned long __v = (unsigned long)(val);                           \
    __asm__ __volatile__("csrs " #csr ", %0" : : "rK"(__v) : "memory"); \
  })


#define clear_csr(csr, val)                                               \
  ({                                                                    \
    unsigned long __v = (unsigned long)(val);                           \
    __asm__ __volatile__("csrc " #csr ", %0" : : "rK"(__v) : "memory"); \
  })
