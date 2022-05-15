/* pte_types.h  */
#ifndef NK_COMPAT_PTE_H
#define NK_COMPAT_PTE_H

#include <nautilus/scheduler.h>


//this is duplicate but to be independent copy the struct here

typedef enum { ARRIVED=0,          // no admission control done
               ADMITTED,           // admitted
               CHANGING,           // admitted for new constraints, now changing to them
               YIELDING,           // explit yield of slice
               SLEEPING,           // being removed from RT and non-RT run/arrival queues
                                   // probably due to having been put into a wait queue
               EXITING,            // being removed from RT and non-RT run/arrival queues
                                   // will not return
               DENIED,             // not admitted
               REAPABLE,           // it's OK for the reaper to destroy the thread
             } rt_status;

typedef enum { RUNNABLE_QUEUE = 0,
               PENDING_QUEUE = 1,
               APERIODIC_QUEUE = 2} queue_type;

typedef struct nk_sched_thread_state {
    // how this thread is to be scheduled
    struct nk_sched_constraints constraints;
    // 
    rt_status status;
    // which queue the thread is currently on
    queue_type q_type;
    
    int      is_intr;      // this is an interrupt thread
    int      is_task;      // this is a task thread

    uint64_t start_time;   // when last started
    uint64_t cur_run_time; // how long it has run without being preempted
    uint64_t run_time;     // how long it has run so far
                           // full duration for aperiodic and sporadic
                           // current slice for periodic
    uint64_t deadline;     // current deadline / time of next arrival if pending
                           // for an aperiodic task, this is its current dynamic
    uint64_t exit_time;    // time of actual completion / arrival

    // Statistics are reset when the constraints are changed
    uint64_t arrival_count;   // how many times it has arrived (1 for aperiodic/sporadic)
    uint64_t resched_count;   // how many times resched was invoked on this thread
    uint64_t resched_long_count; // how many times the long path was taken for the thread
    uint64_t switch_in_count; // number of times switched to
    uint64_t miss_count;      // number of deadline misses
    uint64_t miss_time_sum;   // sum of missed time
    uint64_t miss_time_sum2;  // sum of squares of missed time

    // the thread context itself
    struct nk_thread *thread;

    // the thread node in a thread list (the global thread list)
    struct rt_node   *list; 

} rt_thread ;

typedef struct nk_sched_thread_state rt_thread;
typedef struct rt_node {
    rt_thread      *thread;
    struct rt_node *next;
    struct rt_node *prev;
} rt_node;


#endif
