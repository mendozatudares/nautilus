#include <nautilus/nautilus.h>
#include <nautilus/acpi.h>
#include <nautilus/smp.h>
#include <nautilus/sfi.h>
#include <nautilus/irq.h>
#include <nautilus/mm.h>
#include <nautilus/percpu.h>
#include <nautilus/numa.h>
#include <nautilus/cpu.h>

#ifndef NAUT_CONFIG_DEBUG_SMP
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif
#define SMP_PRINT(fmt, args...) printk("SMP: " fmt, ##args)
#define SMP_DEBUG(fmt, args...) DEBUG_PRINT("SMP: " fmt, ##args)
#define SMP_ERROR(fmt, args...) ERROR_PRINT("SMP: " fmt, ##args)

static int
cpu_init (struct sys_info * sys) {
    struct cpu * new_cpu = NULL;

    if (sys->num_cpus == NAUT_CONFIG_MAX_CPUS) {
        panic("CPU count exceeded max (check your .config)\n");
    }

    if(!(new_cpu = mm_boot_alloc(sizeof(struct cpu)))) {
        panic("Couldn't allocate CPU struct\n");
    } 

    memset(new_cpu, 0, sizeof(struct cpu));

    new_cpu->id         = sys->num_cpus;

    new_cpu->enabled    = 1;
    new_cpu->is_bsp     = 1;
    new_cpu->system     = sys;

    SMP_DEBUG("CPU %d\n", new_cpu->id);
    SMP_DEBUG("\tEnabled?=%d\n", new_cpu->enabled);
    SMP_DEBUG("\tBSP?=%d\n", new_cpu->is_bsp);

    spinlock_init(&new_cpu->lock);

    sys->cpus[sys->num_cpus] = new_cpu;

    sys->num_cpus++;

    return 0;
}

int 
arch_early_init (struct naut_info * naut)
{
    int ret;

    if (cpu_init(&naut->sys) == 0) {
        SMP_DEBUG("CPU init succeeded\n");
        goto out_ok;
    } else {
        panic("CPU init failed. Cannot detect CPUs. Giving up.\n");
    }
			
out_ok:
    SMP_PRINT("Detected %d CPUs\n", naut->sys.num_cpus);
    return 0;
}
