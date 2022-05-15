/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2017, Peter Dinda
 * Copyright (c) 2017, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#ifndef __NK_TASK
#define __NK_TASK

// placed here in case we decide to move more of the
// task implementation into inline
#define TASK_INFO(fmt, args...) INFO_PRINT("task: " fmt, ##args)
#define TASK_ERROR(fmt, args...) ERROR_PRINT("task: " fmt, ##args)
#ifdef NAUT_CONFIG_DEBUG_TASKS
#define TASK_DEBUG(fmt, args...) DEBUG_PRINT("task: " fmt, ##args)
#else
#define TASK_DEBUG(fmt, args...)
#endif
#define TASK_WARN(fmt, args...)  WARN_PRINT("task: " fmt, ##args)



// size of this structure under different cases
// 6*8 = 48 bytes basic stats, 10*8=80 bytes deep stats
struct nk_task_stats {
    uint64_t    size_ns;       // a size of zero means the task has unknown size
    uint64_t    enqueue_time_ns;
    uint64_t    dequeue_time_ns;
    uint64_t    complete_time_ns;
    uint64_t    wait_start_ns;
    uint64_t    wait_end_ns;
#ifdef NAUT_CONFIG_TASK_DEEP_STATISTICS
    uint64_t    create_cost_cycles;
    uint64_t    enqueue_cost_cycles;
    uint64_t    dequeue_cost_cycles;
    uint64_t    destroy_cost_cycles;
#endif
};
    

// size of this structure under different cases
// basic stats, no reuse: (6+6)*8  = 96 bytes   (allocates 128)
// basic states, reuse:   (6+7)*8  = 104 bytes  (allocates 128)
// deep stats, no reuse:  (10+6)*8 = 128 bytes  (allocates 128)
// deep stats, reuse:     (10+7)*8 = 136 bytes  (allocates 256)
struct nk_task {
    // flags are written only on completion, so this can be used
    uint64_t    flags;
#define NK_TASK_COMPLETED 1ULL // set when done
#define NK_TASK_DETACHED  2ULL // no one will wait on this task

    struct nk_task_stats stats;
    
    // a task is a deferred procedure call with
    // this function and this input
    void * (*func)(void *);
    void *input;
    void *output;

    // for managing tasks on queues
    // A task is either in task queue, a reuse stack
    // or in the process of being created or destroyed
    struct list_head queue_node;

#ifdef NAUT_CONFIG_TASK_REUSE
    int alloc_cpu; // the cpu for which this task has allocation affinity
#endif
};

// a CPU-level snapshot - this is APPROXIMATE
typedef struct nk_task_cpu_snapshot {
    uint64_t           sized_enqueued;   // number of sized tasks enqueued
    uint64_t           sized_dequeued;   //   and dequeued (locally or remotely)
                                         // difference is how many are waiting
    uint64_t           unsized_enqueued; // number of unsized tasks enqueud
    uint64_t           unsized_dequeued; //   and dequeued (locally or remotely)
                                         // difference is how many are waiting
} nk_task_cpu_snapshot_t;

// a system-level snapshot - again, APPROXIMATE
typedef nk_task_cpu_snapshot_t nk_task_system_snapshot_t;

    
// create and queue a task
// cpu == -1 => any cpu
// size == 0 => unknown size, otherwise worst case run time in ns
// null return indicated the task cannot be queued
struct nk_task *nk_task_produce(int cpu, uint64_t size_ns, void * (*f)(void*), void *input, uint64_t flags);

// dequeue a task, typically used internally
// dequeuing a task does not execute it.
// cpu = -1 => any cpu
// size = 0 => unsized first, then sized
// size > 0 => search sized queue for up to search_limit steps
struct nk_task *nk_task_consume(int cpu, uint64_t size, uint64_t search_limit);

// same as above, but do not spin
struct nk_task *nk_task_try_consume(int cpu, uint64_t size, uint64_t search_limit);

// mark a task as completed
// this will delete the task if it's detached
int nk_task_complete(struct nk_task *task, void *output);

// wait for a task to complete and optionally get its output and performance stats
// detached tasks cannnot be waited on
// this will delete the task
int nk_task_wait(struct nk_task *task, void **output, struct nk_task_stats *stats);

// same as above, but will not spin
int nk_task_try_wait(struct nk_task *task, void **output, struct nk_task_stats *stats);

// approximate number of tasks waiting to run on on given cpu
void nk_task_cpu_snapshot(int cpu, nk_task_cpu_snapshot_t *snap);

// approximate number of tasks waiting to run on all cpus
void nk_task_system_snapshot(nk_task_system_snapshot_t *snap, uint64_t *idle_cpu_count);

// restrict CPUs that are allowed to consume tasks
// After calling this, you cannot produce a task for an out-of-range CPU
// and you cannot consume a task on an out-of-range CPU.
// No synchronization is done - it's the caller's responsibility to
// drain all tasks before calling this function.   If you don't,
// a race condition could leave a task stranded on an out-of-range CPU
int  nk_task_cpu_restrict(uint64_t first_cpu, uint64_t num_cpus);

// get current CPU restrictions
int  nk_task_cpu_get_restrict(uint64_t *first_cpu, uint64_t *num_cpus);

// use cpu=-1 for state on all cpus
void nk_task_dump_state(int cpu);


// Note that the implementation of tasks in scheduler.c

// used for task runners elsewhere in kernel
#ifdef NAUT_CONFIG_TASK_TRAMPOLINE
void *nk_task_trampoline(void * (*task)(void *), void*);
#define NK_TASK_RUN(t) nk_task_trampoline((t)->func,(t)->input)
#else
#define NK_TASK_RUN(t) (t)->func((t)->input)
#endif

#endif
