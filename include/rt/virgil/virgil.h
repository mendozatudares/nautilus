#ifndef __NK_VIRGIL

// NK's implementation of a VIRGIL-like interface
// is built on the following elements:
//
// NK's task framework is used to implement the worker pool.  The
// kernel is configured with the task framework included and with each
// CPU running a task thread (NAUT_CONFIG_TASK_THREAD). The task
// thread is the worker from the VIRGIL perspective.  The CPU's task
// thread does not migrate.  The VIRGIL interface can create tasks for
// specific CPUs, or for any CPU.  In the later case, tasks are
// currently assigned randomly to CPUs.  Tasks do not currently
// migrate.  A CPU's task thread (worker) consumes tasks in fifo order.
// The task framework assumes tasks do not block, or, if they do, that
// it is acceptable that the task thread running the task will block.
//
// VIRGIL's queues, specifically MARC::ThreadSafeNautilusQueue<T>,
// are used for intertask communication.
//
// VIRGIL's spinlocks are a thin layer on top of NK's spinlocks
//

// C interfaces are here

/*

  Initial compilation/run process

  1. Noelle-side compilation as usual with these changes:
   
- Use of the compiler flags documented in include/rt/virgil/compiler_flags
- Entry point is not main, but rather nk_virgil_entry(int argc, char *argv[])
- All code is compiled to static library => /my/path/to/libvirgil_app.a
- No use of libc or other library primitives that assume the system call interface

  2. Enable Virgil run-time ("make menuconfig" -> Runtimes->Virgil RT) and add library 
     to Runtimes->Virgil->Path to application code 

  3. Compile + link NK

  4. You can now run the application from the command line as:
     virgil command args...

 */


#ifdef __cplusplus

// the thread safe queues with a C++ interface
#include <rt/virgil/ThreadSafeNautilusQueue.hpp>

extern "C" {

#endif

// a task is opaque from the user's perspective
typedef void *nk_virgil_task_t;

// a task runs a callback function pointer of the following form.
// the input pointer points to an opaque quantity, while the return
// value is a pointer to an opaque quantity
typedef void * (*nk_virgil_func_t)(void *input);

// number of workers / number of cpus in system
int nk_virgil_get_num_cpus(void);

// number of workers / number of cpus currently available
// If this returns -1 => error
// if it returns 0, the cpus from *first_cpu to *first_cpu + *num_cpus-1 are available
int nk_virgil_get_num_available_cpus(uint32_t *first_cpu, uint32_t *num_cpus);

// submit a task to any cpu
nk_virgil_task_t nk_virgil_submit_task_to_any_cpu(nk_virgil_func_t func,
						  void *input);

// submit a task to any cpu and detach
nk_virgil_task_t nk_virgil_submit_task_to_any_cpu_and_detach(nk_virgil_func_t func,
							     void *input);

// submit a task to a specific cpu
nk_virgil_task_t nk_virgil_submit_task_to_specific_cpu(nk_virgil_func_t func,
						       void *input,
						       int cpu);

// submit a task to any one of a set of cpus
nk_virgil_task_t nk_virgil_submit_task_to_cpu_set(nk_virgil_func_t func,
						  void *input,
						  cpu_set_t *cpuset);


// immediately returns
//    <0 => error
//    =0 => task is not yet done
//    >0 => task is done, *output contains its output pointer
//          [in this case, do not check on the task again]
int nk_virgil_check_for_task_completion(nk_virgil_task_t task, void **output);

// waits for task completion or error
//    <0 => error
//     0 => should not happen
//    >0 => task is done, *output contains its output pointer
//          [in this case, do not check on the task again]
int nk_virgil_wait_for_task_completion(nk_virgil_task_t task, void **output);

// get number of idle cpus (ones without tasks)
// note that there is no way to make this scalable...
// negative return => error
int nk_virgil_idle_cpus(void);

// get the number of tasks that are queued on a given cpu
// negative return => error
int nk_virgil_waiting_tasks_cpu(int cpu);

// get the number of tasks that are queued throughout the system
// note that there is no way to make this scalable...
// negative return => error
int nk_virgil_waiting_tasks_system(void);


   
// spinlocks - should be identical type to spinlock_t
typedef unsigned int nk_virgil_spinlock_t;

// initialize a lock to the unlocked state
void nk_virgil_spinlock_init(nk_virgil_spinlock_t *lock);
void nk_virgil_spinlock_deinit(nk_virgil_spinlock_t *lock);

// spin on the lock until you get it
void nk_virgil_spinlock_lock(nk_virgil_spinlock_t *lock);
    
// attempt to get the lock, just once, returns zero on success
int nk_virgil_spinlock_try_lock(nk_virgil_spinlock_t *lock);

void nk_virgil_spinlock_unlock(nk_virgil_spinlock_t *lock);

// the irq_save/irq_restore aspects are not used in virgil at the current time

#ifdef __cplusplus
}
#endif



#endif















