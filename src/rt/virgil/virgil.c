#include <nautilus/nautilus.h>
#include <nautilus/task.h>
#include <nautilus/shell.h>

#define _GNU_SOURCE
#include <sched.h>   // to get us the cpu-set macros

#include <rt/virgil/virgil.h>


#define INFO(fmt, args...) INFO_PRINT("virgil: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("virgil: " fmt, ##args)
#ifdef NAUT_CONFIG_VIRGIL_RT_DEBUG
#define DEBUG(fmt, args...) DEBUG_PRINT("virgil: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define WARN(fmt, args...)  WARN_PRINT("virgil: " fmt, ##args)

int nk_virgil_get_num_cpus(void)
{
    return nk_get_num_cpus();
}

int nk_virgil_get_num_available_cpus(uint32_t *first_cpu, uint32_t *num_cpus)
{
  uint64_t first, num;
  int rc = nk_task_cpu_get_restrict(&first,&num);
  *first_cpu = (uint32_t) first;  // yuck, I know
  *num_cpus = (uint32_t) num;
  return rc;
}


// submit a task to any cpu
nk_virgil_task_t nk_virgil_submit_task_to_any_cpu(nk_virgil_func_t func,
						  void *input)
{
    // any cpu, unknown size, not detached
    DEBUG("submit virgil task %p(%p) to any cpu\n", func, input);
    nk_virgil_task_t task = nk_task_produce(-1,0,func,input,0);
    DEBUG("resulting NK task is %p\n", task);
    return task;
}

// submit a task to any cpu and detach
nk_virgil_task_t nk_virgil_submit_task_to_any_cpu_and_detach(nk_virgil_func_t func,
							     void *input)
{
    // any cpu, unknown size, detached
    DEBUG("submit virgil task %p(%p) to any cpu and detach\n", func, input);
    nk_virgil_task_t task = nk_task_produce(-1,0,func,input,NK_TASK_DETACHED);
    DEBUG("resulting NK task is %p\n", task);
    return task;
}

// submit a task to a specific cpu
nk_virgil_task_t nk_virgil_submit_task_to_specific_cpu(nk_virgil_func_t func,
						       void *input,
						       int cpu)
{
    // specific cpu, unknown size, not detached
    DEBUG("submit virgil task %p(%p) to cpu %d\n", func, input, cpu);
    nk_virgil_task_t task = nk_task_produce(cpu,0,func,input,0);
    DEBUG("resulting NK task is %p\n", task);
    return task;
}


// submit a task to any one of a set of cpus
nk_virgil_task_t nk_virgil_submit_task_to_cpu_set(nk_virgil_func_t func,
						  void *input,
						  cpu_set_t *cpuset)
{
    // we will define this to mean pick one at random from the set
    int i;
    int num=0;
    
    for (i=0; i<CPU_SETSIZE;i++) {
	// we cannot use CPU_COUNT here since it is not a macro
	if (CPU_ISSET(i,cpuset)) {
	    num++;
	}
    }
    
    int target = rdtsc()%num;
    int cpus;
    int last;

    DEBUG("submit virgil task %p(%p) to cpu set - %d cpus in the set, our target is %d\n",func, input, num,target);
    
    cpus=0;
    last=0;
    for (i=0; i<CPU_SETSIZE;i++) {
	if (CPU_ISSET(i,cpuset)) {
	    last = i;
	    cpus++;
	    if (cpus==target) {
		break;
	    }
	}
    }

    if (cpus==target) {
	DEBUG("target cpu is %d\n",i);
	nk_virgil_task_t task = nk_virgil_submit_task_to_specific_cpu(func,input,i);
	DEBUG("resulting NK task is %p\n", task);
	return task;
    } else {
	ERROR("target NOT found\n");
	return 0;
    }
}


// immediately returns
//    <0 => error
//    =0 => task is not yet done
//    >0 => task is done, *output contains its output pointer
//          [in this case, do not check on the task again]
int nk_virgil_check_for_task_completion(nk_virgil_task_t task, void **output)
{
    DEBUG("try wait on task %p\n", task);

    // no statistics requested    
    int rc = nk_task_try_wait((struct nk_task *)task, output, 0);

    DEBUG("try wait result is %d\n", rc);
    
    if (rc<0) {
	return rc;
    } else {
	// swap sense
	return !rc;
    }
}


// waits for task completion or error
//    <0 => error
//     0 => should not happen
//    >0 => task is done, *output contains its output pointer
//          [in this case, do not check on the task again]
int nk_virgil_wait_for_task_completion(nk_virgil_task_t task, void **output)
{
#define STATS 0

    DEBUG("wait on task %p\n", task);
 
#ifdef STATS  
    struct nk_task_stats stats;
    int rc = nk_task_wait((struct nk_task *)task,output,&stats);
#else
    int rc = nk_task_wait((struct nk_task *)task,output,0);
#endif

    DEBUG("wait result is %d\n", rc);

    if (rc<0) {
	return rc;
    } else {
#ifdef STATS
        DEBUG("task completion: task %p, size %lu ns, enqueued at %lu ns, dequeued at %lu ns, completed at %lu ns, wait start at %lu ns, wait end at %lu ns\n",
  	      task,stats.size_ns,stats.enqueue_time_ns,stats.dequeue_time_ns,stats.complete_time_ns,stats.wait_start_ns, stats.wait_end_ns);
        DEBUG("task completion: task %p, queue wait time: %lu ns, service time (approx) %lu ns, response time %lu ns, wait time: %lu ns\n",
	    task,stats.dequeue_time_ns - stats.enqueue_time_ns,
	    stats.complete_time_ns - stats.dequeue_time_ns,
	    stats.complete_time_ns - stats.enqueue_time_ns,
	    stats.wait_end_ns - stats.wait_start_ns);
#endif
	// swap sense
	return !rc;
    }
}

// get the number of tasks that are queued on a given cpu
// negative return => error
int nk_virgil_waiting_tasks_cpu(int cpu)
{
    nk_task_cpu_snapshot_t snap;

    DEBUG("get count of waiting tasks on cpu %d\n", cpu);
    
    nk_task_cpu_snapshot(cpu,&snap);

    uint64_t count;
    
    // we assume we are responsible for all the unsized tasks
    if (snap.unsized_dequeued <= snap.unsized_enqueued) {
	count = (snap.unsized_enqueued - snap.unsized_dequeued);
    } else {
	count = 0;
    }
    
    DEBUG("count is %lu (returning %d)\n", count, (int)count);
    return count;
}


static void _waiting_info_sys(uint64_t *waiting, uint64_t *idle_cpus)
{
    nk_task_system_snapshot_t snap;

    DEBUG("get count of waiting tasks and idle cpus on system\n");
    
    nk_task_system_snapshot(&snap,idle_cpus);

    // we assume we are responsible for all the unsized tasks
    if (snap.unsized_dequeued <= snap.unsized_enqueued) {
	*waiting = (snap.unsized_enqueued - snap.unsized_dequeued);
    } else {
	*waiting = 0;
    }
    DEBUG("count of waiting tasks is %lu and there are %lu idle cpus\n", *waiting, *idle_cpus);
}


// get the number of tasks that are queued throughout the system
// note that there is no way to make this scalable...
// negative return => error
int nk_virgil_waiting_tasks_on_system(int cpu)
{
    uint64_t waiting, idle;
    
    _waiting_info_sys(&waiting,&idle);

    DEBUG("there are %d waiting tasks on the system\n", (int)waiting);
    
    return (int)waiting;
}


// get number of idle cpus (ones without tasks)
// note that there is no way to make this scalable...
// negative return => error
int nk_virgil_idle_cpus(void)
{
    uint64_t waiting, idle;
    
    _waiting_info_sys(&waiting,&idle);

    DEBUG("there are %d idle cpus in the system\n", (int)idle);
    
    return (int)idle;
}






// initialize a lock to the unlocked state
void nk_virgil_spinlock_init(nk_virgil_spinlock_t *lock)
{
    spinlock_init(lock);
}
 
void nk_virgil_spinlock_deinit(nk_virgil_spinlock_t *lock)
{
    spinlock_deinit(lock);
}

// spin on the lock until you get it
void nk_virgil_spinlock_lock(nk_virgil_spinlock_t *lock)
{
    spin_lock(lock);
}
    
// attempt to get the lock, just once, returns zero on success
int nk_virgil_spinlock_try_lock(nk_virgil_spinlock_t *lock)
{
    return spin_try_lock(lock);
}
 

void nk_virgil_spinlock_unlock(nk_virgil_spinlock_t *lock)
{
    spin_unlock(lock);
}



static int _nk_virgil_entry_test(int argc, char *argv[])
{
    int i;
    DEBUG("hello, my args are:\n");
    for (i=0;i<argc;i++) {
      DEBUG("argv[%d] = \"%s\"\n",i,argv[i]);
    }

    return 0;
}

int smain(int argc, char *argv[]);

#define MAXCMD 256

static int
handle_virgil(char * buf, void * priv)
{
    char b[MAXCMD];
    char *argv[MAXCMD];
    int argc=0;
    uint64_t numcpus;

    if (sscanf(buf,"virgil _numcpus %lu",&numcpus)==1) {
      nk_vc_printf("using CPUs [0,%lu)\n",numcpus);
      if (nk_task_cpu_restrict(0,numcpus)) {
	nk_vc_printf("cannot apply restriction\n");
      }
      return 0;
    }
    
    strncpy(b,buf,MAXCMD); b[MAXCMD-1]=0;

    char *c = b;
    enum {WS,NWS} state=WS;

    // we are converting
    //    virgil cmd arg1 arg2 arg3
    // to
    //    virgil\0cmd\0arg1\0\arg2\0\arg3\0
    //

    while (*c) {
	if (state==WS) {
	    if (isspace(*c)) {
		// nothing
	    } else {
	        argv[argc++] = c;
		state = NWS;
	    }
	} else { //state==NWS
	    if (isspace(*c)) {
		*c = 0;
		state = WS;
	    } else {
		//nothing
	    }
	}
	c++;
    }

    char **dest_argv = &argv[1];
    int dest_argc = argc-1;
    
    //    int rc = _nk_virgil_entry_test(dest_argc,dest_argv);
    _nk_virgil_entry_test(dest_argc,dest_argv);
    int rc = smain(dest_argc,dest_argv);
    

    nk_vc_printf("virgil app returned %d\n", rc);
    
    
    return 0;
}


static struct shell_cmd_impl virgil_impl = {
    .cmd      = "virgil",
    .help_str = "virgil command args...",
    .handler  = handle_virgil,
};
nk_register_shell_cmd(virgil_impl);
















