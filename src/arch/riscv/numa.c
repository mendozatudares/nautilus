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
 * http://xtack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/numa.h>
#include <nautilus/mm.h>
#include <nautilus/macros.h>
#include <nautilus/errno.h>
#include <nautilus/acpi.h>
#include <nautilus/list.h>
#include <nautilus/devicetree.h>

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

static int srat_rev;
extern off_t dtb_ram_start;
extern size_t dtb_ram_size;


#ifndef NAUT_CONFIG_DEBUG_NUMA
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif
#define NUMA_PRINT(fmt, args...) printk("NUMA: " fmt, ##args)
#define NUMA_DEBUG(fmt, args...) DEBUG_PRINT("NUMA: " fmt, ##args)
#define NUMA_ERROR(fmt, args...) ERROR_PRINT("NUMA: " fmt, ##args)
#define NUMA_WARN(fmt, args...)  WARN_PRINT("NUMA: " fmt, ##args)


static inline int
domain_exists (struct sys_info * sys, unsigned id)
{
    return sys->locality_info.domains[id] != NULL;
}

static inline struct numa_domain *
get_domain (struct sys_info * sys, unsigned id)
{
    return sys->locality_info.domains[id];
}

struct numa_domain *
nk_numa_domain_create (struct sys_info * sys, unsigned id)
{
    struct numa_domain * d = NULL;

    d = (struct numa_domain *)mm_boot_alloc(sizeof(struct numa_domain));
    if (!d) {
        ERROR_PRINT("Could not allocate NUMA domain\n");
        return NULL;
    }
    memset(d, 0, sizeof(struct numa_domain));

    d->id = id;

    INIT_LIST_HEAD(&(d->regions));
    INIT_LIST_HEAD(&(d->adj_list));

    if (id != (sys->locality_info.num_domains + 1)) { 
        NUMA_DEBUG("Memory regions are not in expected domain order, but that should be OK\n");
    }

    sys->locality_info.domains[id] = d;
    sys->locality_info.num_domains++;

    return d;
}

/* 
 * Assumes that nk_acpi_init() has 
 * already been called 
 *
 */
int 
arch_numa_init (struct sys_info * sys)
{
    NUMA_PRINT("Parsing ACPI NUMA information...\n");

    // /* SLIT: System Locality Information Table */
    // if (acpi_table_parse(ACPI_SIG_SLIT, acpi_parse_slit, &(sys->locality_info))) { 
    //     NUMA_DEBUG("Unable to parse SLIT\n");
    // }

    // /* SRAT: Static Resource Affinity Table */
    // if (!acpi_table_parse(ACPI_SIG_SRAT, acpi_parse_srat, &(sys->locality_info))) {

    //     NUMA_DEBUG("Parsing SRAT_MEMORY_AFFINITY table...\n");

    //     if (acpi_table_parse_srat(ACPI_SRAT_MEMORY_AFFINITY,
    //                 acpi_parse_memory_affinity, NAUT_CONFIG_MAX_CPUS * 2) < 0) {
    //         NUMA_ERROR("Unable to parse memory affinity\n");
    //     }

    //     NUMA_DEBUG("DONE.\n");

    //     NUMA_DEBUG("Parsing SRAT_TYPE_X2APIC_CPU_AFFINITY table...\n");

    //     if (acpi_table_parse_srat(ACPI_SRAT_X2APIC_CPU_AFFINITY,
    //                 acpi_parse_x2apic_affinity, NAUT_CONFIG_MAX_CPUS) < 0)	    {
    //         NUMA_ERROR("Unable to parse x2apic table\n");
    //     }

    //     NUMA_DEBUG("DONE.\n");

    //     NUMA_DEBUG("Parsing SRAT_PROCESSOR_AFFINITY table...\n");

    //     if (acpi_table_parse_srat(ACPI_SRAT_PROCESSOR_AFFINITY,
    //                 acpi_parse_processor_affinity,
    //                 NAUT_CONFIG_MAX_CPUS) < 0) { 
    //         NUMA_ERROR("Unable to parse processor affinity\n");
    //     }

    //     NUMA_DEBUG("DONE.\n");

    // }

    NUMA_WARN("Faking domain 0 because of lack of RISC-V support\n");
    uint32_t i, domain_id = 0;
	struct numa_domain *n = nk_numa_domain_create(sys, domain_id);
	if (!n) {
	    NUMA_ERROR("Cannot allocate fake domain 0\n");
	    return -1;
	}

    struct mem_region *mem = mm_boot_alloc(sizeof(struct mem_region));
    if (!mem) {
        ERROR_PRINT("Could not allocate mem region\n");
        return -1;
    }
    memset(mem, 0, sizeof(struct mem_region));
    
    mem->domain_id     = domain_id;
    mem->base_addr     = dtb_ram_start;
    mem->len           = dtb_ram_size;
    mem->enabled       = 1;


    n->num_regions++;
    n->addr_space_size += mem->len;

    NUMA_DEBUG("Domain %u now has 0x%lx regions and size 0x%lx\n", n->id, n->num_regions, n->addr_space_size);

    if (list_empty(&n->regions)) {
        list_add(&mem->entry, &n->regions);
    } else {
        list_add_tail(&mem->entry, &n->regions);
    }

    for (i = 0; i < sys->num_cpus; i++) {
        sys->cpus[i]->domain = get_domain(sys, domain_id);
    }

    NUMA_PRINT("DONE.\n");

    return 0;
}
