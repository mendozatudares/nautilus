#include <nautilus/nautilus.h>
#include <nautilus/acpi.h>
#include <nautilus/smp.h>
#include <nautilus/sfi.h>
#include <nautilus/irq.h>
#include <nautilus/mm.h>
#include <nautilus/percpu.h>
#include <nautilus/numa.h>
#include <nautilus/cpu.h>
#include <nautilus/devicetree.h>

#ifndef NAUT_CONFIG_DEBUG_SMP
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif
#define SMP_PRINT(fmt, args...) printk("SMP: " fmt, ##args)
#define SMP_DEBUG(fmt, args...) DEBUG_PRINT("SMP: " fmt, ##args)
#define SMP_ERROR(fmt, args...) ERROR_PRINT("SMP: " fmt, ##args)

static struct sys_info * sys;

static int
dtb_parse_cpu (struct dtb_node * n) {
    struct cpu * new_cpu = NULL;

    if (sys->num_cpus == NAUT_CONFIG_MAX_CPUS) {
        panic("CPU count exceeded max (check your .config)\n");
    }

    if(!(new_cpu = mm_boot_alloc(sizeof(struct cpu)))) {
        panic("Couldn't allocate CPU struct\n");
    } 

    memset(new_cpu, 0, sizeof(struct cpu));

    new_cpu->id         = n->reg.address;
    new_cpu->lapic_id   = 0;

    new_cpu->enabled    = 1;
    new_cpu->is_bsp     = (new_cpu->id == sys->bsp_id ? 1 : 0);
    new_cpu->cpu_sig    = 0;
    new_cpu->feat_flags = 0;
    new_cpu->system     = sys;
    new_cpu->cpu_khz    = 0;

    SMP_DEBUG("CPU %u\n", new_cpu->id);
    SMP_DEBUG("\tEnabled?=%01d\n", new_cpu->enabled);
    SMP_DEBUG("\tBSP?=%01d\n", new_cpu->is_bsp);

    spinlock_init(&new_cpu->lock);

    sys->cpus[new_cpu->id] = new_cpu;
    sys->num_cpus++;

    return 0;
}

static int
dtb_parse_plic (struct dtb_node * n) {
    struct ioapic * ioa = NULL;

    if (sys->num_ioapics == NAUT_CONFIG_MAX_IOAPICS) {
        panic("IOAPIC count exceeded max (change it in .config)\n");
    }

    if (!(ioa = mm_boot_alloc(sizeof(struct ioapic)))) {
        panic("Couldn't allocate IOAPIC struct\n");
    }
    memset(ioa, 0, sizeof(struct ioapic));

    ioa->id      = 0;
    ioa->version = 0;
    ioa->usable  = 1;
    ioa->base    = (addr_t)n->reg.address;

    SMP_DEBUG("IOAPIC entry:\n");
    SMP_DEBUG("\tID=0x%x\n", ioa->id);
    SMP_DEBUG("\tVersion=0x%x\n", ioa->version);
    SMP_DEBUG("\tEnabled?=%01d\n", ioa->usable);
    SMP_DEBUG("\tBase Addr=0x%lx\n", ioa->base);

    sys->ioapics[sys->num_ioapics] = ioa;
    sys->num_ioapics++;

    return 0;
}

bool_t dtb_node_get_cpu (struct dtb_node * n) {
    if(!strcmp(n->name, "cpu")) {
        dtb_parse_cpu(n);
    } else if(!strcmp(n->name, "interrupt-controller") && strstr(n->compatible, "plic0")) {
        dtb_parse_plic(n);
    }
    return true;
}

static int __early_init_dtb(struct naut_info * naut) {
    sys = &(naut->sys);
    dtb_walk_devices(dtb_node_get_cpu);
    return 0;
}

int 
arch_early_init (struct naut_info * naut)
{
    int ret;

    SMP_DEBUG("Checking for DTB\n");

    if (__early_init_dtb(naut) == 0) {
        SMP_DEBUG("DTB init succeeded\n");
        goto out_ok;
    } else {
        panic("DTB not present/working! Cannot detect CPUs. Giving up.\n");
    }

out_ok:
    SMP_PRINT("Detected %d CPUs\n", naut->sys.num_cpus);
    return 0;
}

/* Some fakery to get the scheduler working */
uint8_t
nk_topo_cpus_share_socket (struct cpu * a, struct cpu * b)
{
    return 1;
    // return a->coord->pkg_id == b->coord->pkg_id;
}

uint8_t
nk_topo_cpus_share_phys_core (struct cpu * a, struct cpu * b)
{
    return nk_topo_cpus_share_socket(a, b) && (a->coord->core_id == b->coord->core_id);
}
