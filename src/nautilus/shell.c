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
 * Copyright (c) 2016, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2016, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Peter Dinda <pdinda@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/vc.h>
#ifdef NAUT_CONFIG_PALACIOS
#include <nautilus/vmm.h>
#endif

#define MAX_CMD 80

struct burner_args {
    struct nk_virtual_console *vc;
    char     name[MAX_CMD];
    uint64_t size_ns; 
    struct nk_sched_constraints constraints;
} ;

static void burner(void *in, void **out)
{
    uint64_t start, end, dur;
    struct burner_args *a = (struct burner_args *)in;

    nk_thread_name(get_cur_thread(),a->name);

    if (nk_bind_vc(get_cur_thread(), a->vc)) { 
	ERROR_PRINT("Cannot bind virtual console for burner %s\n",a->name);
	return;
    }

    nk_vc_printf("%s (tid %llu) attempting to promote itself\n", a->name, get_cur_thread()->tid);
#if 1
    if (nk_sched_thread_change_constraints(&a->constraints)) { 
	nk_vc_printf("%s (tid %llu) rejected - exiting\n", a->name, get_cur_thread()->tid);
	return;
    }
#endif

    nk_vc_printf("%s (tid %llu) promotion complete - spinning for %lu ns\n", a->name, get_cur_thread()->tid, a->size_ns);

    while(1) {
	start = nk_sched_get_realtime();
	udelay(1000);
	end = nk_sched_get_realtime();
	dur = end - start;
	//	nk_vc_printf("%s (tid %llu) start=%llu, end=%llu left=%llu\n",a->name,get_cur_thread()->tid, start, end,a->size_ns);
	if (dur >= a->size_ns) { 
	    nk_vc_printf("%s (tid %llu) done - exiting\n",a->name,get_cur_thread()->tid);
	    free(in);
	    return;
	} else {
	    a->size_ns -= dur;
	}
    }
}

static int launch_aperiodic_burner(char *name, uint64_t size_ns, uint64_t priority)
{
    nk_thread_id_t tid;
    struct burner_args *a;

    a = malloc(sizeof(struct burner_args));
    if (!a) { 
	return -1;
    }
    
    strncpy(a->name,name,MAX_CMD); a->name[MAX_CMD-1]=0;
    a->vc = get_cur_thread()->vc;
    a->size_ns = size_ns;
    a->constraints.type=APERIODIC;
    a->constraints.aperiodic.priority=priority;

    nk_thread_start(burner, (void*)a , NULL, 1, PAGE_SIZE, &tid, -1);
    
    return 0;
}

static int launch_sporadic_burner(char *name, uint64_t size_ns, uint64_t phase, uint64_t size, uint64_t deadline, uint64_t aperiodic_priority)
{
    nk_thread_id_t tid;
    struct burner_args *a;

    a = malloc(sizeof(struct burner_args));
    if (!a) { 
	return -1;
    }
    
    strncpy(a->name,name,MAX_CMD); a->name[MAX_CMD-1]=0;
    a->vc = get_cur_thread()->vc;
    a->size_ns = size_ns;
    a->constraints.type=SPORADIC;
    a->constraints.sporadic.phase = phase;
    a->constraints.sporadic.size = size;
    a->constraints.sporadic.deadline = deadline;
    a->constraints.sporadic.aperiodic_priority = aperiodic_priority;

    nk_thread_start(burner, (void*)a , NULL, 1, PAGE_SIZE, &tid, -1);
    
    return 0;
}

static int launch_periodic_burner(char *name, uint64_t size_ns, uint64_t phase, uint64_t period, uint64_t slice)
{
    nk_thread_id_t tid;
    struct burner_args *a;

    a = malloc(sizeof(struct burner_args));
    if (!a) { 
	return -1;
    }
    
    strncpy(a->name,name,MAX_CMD); a->name[MAX_CMD-1]=0;
    a->vc = get_cur_thread()->vc;
    a->size_ns = size_ns;
    a->constraints.type=PERIODIC;
    a->constraints.periodic.phase = phase;
    a->constraints.periodic.period = period;
    a->constraints.periodic.slice = slice;

    nk_thread_start(burner, (void*)a , NULL, 1, PAGE_SIZE, &tid, -1);
    
    return 0;
}

static int handle_cmd(char *buf, int n)
{
  char name[MAX_CMD];
  uint64_t size_ns;
  uint64_t priority, phase;
  uint64_t period, slice;
  uint64_t size, deadline;
  int cpu;

  if (*buf==0) { 
    return 0;
  }

  if (!strncasecmp(buf,"exit",4)) { 
    return 1;
  }
  
  if (!strncasecmp(buf,"help",4)) { 
    nk_vc_printf("help\nexit\nvcs\ncores [n]\ntime [n]\nthreads [n]\n");
    nk_vc_printf("shell name\n");
    nk_vc_printf("reap\n");
    nk_vc_printf("burn a name size_ms priority\n");
    nk_vc_printf("burn s name size_ms phase size deadline priority\n");
    nk_vc_printf("burn p name size_ms phase period slice\n");
    nk_vc_printf("vm name [embedded image]\n");
    return 0;
  }

  if (!strncasecmp(buf,"vcs",3)) {
    nk_switch_to_vc_list();
    return 0;
  }

  if (sscanf(buf,"shell %s", name)==1) { 
    nk_launch_shell(name,-1);
    return 0;
  }

  if (!strncasecmp(buf,"reap",4)) { 
    nk_sched_reap();
    return 0;
  }

  if (sscanf(buf,"burn a %s %llu %llu", name, &size_ns, &priority)==3) { 
    nk_vc_printf("Starting aperiodic burner %s with size %llu ms and priority %llu\n",name,size_ns,priority);
    size_ns *= 1000000;
    launch_aperiodic_burner(name,size_ns,priority);
    return 0;
  }

  if (sscanf(buf,"burn s %s %llu %llu %llu %llu %llu", name, &size_ns, &phase, &size, &deadline, &priority)==6) { 
    nk_vc_printf("Starting sporadic burner %s with size %llu ms phase %llu from now size %llu ms deadline %llu ms from now and priority %lu\n",name,size_ns,phase,size,deadline,priority);
    size_ns *= 1000000;
    phase   *= 1000000; 
    size    *= 1000000;
    deadline*= 1000000; deadline+= nk_sched_get_realtime();
    launch_sporadic_burner(name,size_ns,phase,size,deadline,priority);
    return 0;
  }

  if (sscanf(buf,"burn p %s %llu %llu %llu %llu", name, &size_ns, &phase, &period, &slice)==5) { 
    nk_vc_printf("Starting periodic burner %s with size %llu ms phase %llu from now period %llu ms slice %llu ms\n",name,size_ns,phase,period,slice);
    size_ns *= 1000000;
    phase   *= 1000000; 
    period  *= 1000000;
    slice   *= 1000000;
    launch_periodic_burner(name,size_ns,phase,period,slice);
    return 0;
  }

#ifdef NAUT_CONFIG_PALACIOS_EMBEDDED_VM_IMG
  if (sscanf(buf,"vm %s", name)==1) { 
    extern int guest_start;
    nk_vmm_start_vm(name,&guest_start,0xffffffff);
    return 0;
  }
#endif

  if (!strncasecmp(buf,"threads",7)) { 
    if (sscanf(buf,"threads %d",&cpu)!=1) {
      cpu=-1; 
    }
    nk_sched_dump_threads(cpu);
    return 0;
  }

  if (!strncasecmp(buf,"cores",5)) { 
    if (sscanf(buf,"cores %d",&cpu)!=1) {
      cpu=-1; 
    }
    nk_sched_dump_cores(cpu);
    return 0;
  }

  if (!strncasecmp(buf,"time",4)) { 
    if (sscanf(buf,"time %d",&cpu)!=1) {
      cpu=-1; 
    }
    nk_sched_dump_time(cpu);
    return 0;
  }


  nk_vc_printf("Don't understand \"%s\"\n",buf);
  return 0;
}

static void shell(void *in, void **out)
{
  struct nk_virtual_console *vc = nk_create_vc((char*)in,COOKED, 0x9f, 0, 0);
  char buf[MAX_CMD];
  char lastbuf[MAX_CMD];
  int first=1;

  if (!vc) { 
    ERROR_PRINT("Cannot create virtual console for shell\n");
    return;
  }

  if (nk_thread_name(get_cur_thread(),(char*)in)) {   
    ERROR_PRINT("Cannot name shell's thread\n");
    return;
  }

  if (nk_bind_vc(get_cur_thread(), vc)) { 
    ERROR_PRINT("Cannot bind virtual console for shell\n");
    return;
  }
   
  nk_switch_to_vc(vc);
  
  nk_vc_clear(0x9f);
   
  while (1) {  
    nk_vc_printf("%s> ", (char*)in);
    nk_vc_gets(buf,MAX_CMD,1);

    if (buf[0]==0 && !first) { 
	// continue; // turn off autorepeat for now
	if (handle_cmd(lastbuf,MAX_CMD)) { 
	    break;
	}
    } else {
	if (handle_cmd(buf,MAX_CMD)) {
	    break;
	}
	memcpy(lastbuf,buf,MAX_CMD);
	first=0;

    }
	       
  }

  nk_vc_printf("Exiting shell %s\n", (char*)in);
  free(in);
  nk_release_vc(get_cur_thread());

  // exit thread
  
}

nk_thread_id_t nk_launch_shell(char *name, int cpu)
{
  nk_thread_id_t tid;
  char *n = malloc(32);

  if (!n) {
    return 0;
  }

  strncpy(n,name,32);
  n[31]=0;
  
  nk_thread_start(shell, (void*)n, 0, 1, PAGE_SIZE, &tid, cpu);

  INFO_PRINT("Shell %s launched on cpu %d as %p\n",name,cpu,tid);

  return tid;
}



