#include <nautilus/cpu.h>
#include <nautilus/atomic.h>
#include <nautilus/intrinsics.h>


static inline void
bspin_lock (volatile int * lock)
{
        while (__sync_lock_test_and_set(lock, 1));
}

static inline void
bspin_unlock (volatile int * lock)
{
        __sync_lock_release(lock);
}

static void
barrier_xcall_handler (void * arg)
{
    nk_core_barrier_arrive();
}

