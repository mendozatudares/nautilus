#ifndef _PTHREAD_SIMPLE_SEM_H_
#define _PTHREAD_SIMPLE_SEM_H_
#include<nautilus/spinlock.h>
#include<nautilus/waitqueue.h>
#include "../debug.h"
struct simple_sem{
  spinlock_t lock;
  uint64_t count;
  int sleepcount;
  nk_wait_queue_t *wait_queue;

};


typedef struct simple_sem simple_sem_t;

void ssem_destroy(simple_sem_t *s);

void ssem_init(simple_sem_t *s, int initial_count);

void ssem_post(simple_sem_t *s, int count);

void ssem_wait( simple_sem_t *s);

int ssem_timedwait( simple_sem_t *s, uint64_t timeout_ns);


#endif
