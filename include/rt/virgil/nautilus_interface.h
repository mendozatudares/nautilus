#ifndef _NAUTILUS_INTERFACE
#define _NAUTILUS_INTERFACE

extern "C" {
  // These are duplicated here since if we add
  // the typical nautilus includes, the C++ compiler
  // becomes very unhappy

#define DEBUG_PRINT(fmt, args...) nk_vc_printf("DEBUG: " fmt, ##args)
#define ERROR_PRINT(fmt, args...) nk_vc_printf("ERROR: " fmt, ##args)
#define INFO_PRINT(fmt, args...)  nk_vc_printf(fmt, ##args)
  
#ifdef NAUT_CONFIG_VIRGIL_RT_DEBUG
#define VIRGIL_DEBUG(fmt, args...) DEBUG_PRINT("virgil: " fmt, ##args)
#else 
#define VIRGIL_DEBUG(fmt, args...)
#endif
#define VIRGIL_ERROR(fmt, args...) ERROR_PRINT("virgil: " fmt, ##args)
#define VIRGIL_INFO(fmt, args...)  INFO_PRINT("virgil: " fmt, ##args)

#define LOCK_TYPE      spinlock_t
#define LOCK_INIT(x)   spinlock_init(x)
#define LOCK_DEINIT(x) // empty

// For now we will not disable interrupts
#define DO_IRQ 0

#if DO_IRQ
#define LOCK_DECL      uint8_t _nk_flags;
#define LOCK(x)        _nk_flags = spin_lock_irq_save(x)
#define UNLOCK(x)      spin_unlock_irq_restore(x,_nk_flags)
#else 
#define LOCK_DECL      
#define LOCK(x)        spin_lock(x)
#define UNLOCK(x)      spin_unlock(x)
#endif

#define TSTACK_DEFAULT 0
  
  typedef void (*nk_thread_fun_t)(void * input, void ** output);
  typedef unsigned long nk_stack_size_t; // uint64_t
  typedef void* nk_thread_id_t;

  int nk_thread_start (nk_thread_fun_t fun, 
		       void * input,
		       void ** output,
		       unsigned char is_detached, //uint8_t
		       nk_stack_size_t stack_size,
		       nk_thread_id_t * tid,
		       int bound_cpu); // -1 => not bound

  int nk_join(nk_thread_id_t t, void ** retval);

  int nk_vc_printf(const char* fmt, ...);
  
  unsigned nk_get_num_cpus();

  typedef unsigned spinlock_t; // uint32_t
  
  void spinlock_init(volatile spinlock_t *);

  static inline void spin_lock(volatile spinlock_t *l)
  {
    while (__sync_lock_test_and_set(l,1)) {
    }
  }

  static inline void spin_unlock(volatile spinlock_t *l)
  {
    __sync_lock_release(l);
  }
    
  void nk_yield(void);
  
  void *nk_virgil_get_cur_thread(void);

#if 0
  // *y = *x
#define ATOMIC_LOAD(T, x, y)  __atomic_load(x,y,__ATOMIC_SEQ_CST);

  // *x = *y;
#define ATOMIC_STORE(T, y, x) __atomic_store(x,y,__ATOMIC_SEQ_CST); 

  static inline bool atomic_load_bool(const bool *addr)
  {
    bool val;
    ATOMIC_LOAD(bool,addr,&val);
    return val;
  }
  static inline void atomic_store_bool(bool *addr, bool val)
  {
    ATOMIC_STORE(bool,addr,&val);
  }
    
#else

#define atomic_load_bool(addr) __atomic_load_n(addr,__ATOMIC_SEQ_CST)
#define atomic_store_bool(addr,val) __atomic_store_n(addr,val,__ATOMIC_SEQ_CST)

#endif
  
}

#endif
