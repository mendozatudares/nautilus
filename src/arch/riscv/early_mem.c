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
 * Authors: Kyle C. Hale <kh@u.northwestern.edu>
 *          Peter Dinda <pdinda@northwestern.edu> (Gem5 - e820)
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/mm.h>
#include <nautilus/mb_utils.h>
#include <nautilus/macros.h>
#include <nautilus/multiboot2.h>
#include <nautilus/devicetree.h>

extern char * mem_region_types[6];

#ifndef NAUT_CONFIG_DEBUG_BOOTMEM
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define BMM_DEBUG(fmt, args...) DEBUG_PRINT("BOOTMEM: " fmt, ##args)
#define BMM_PRINT(fmt, args...) printk("BOOTMEM: " fmt, ##args)
#define BMM_WARN(fmt, args...)  WARN_PRINT("BOOTMEM: " fmt, ##args)

extern ulong_t kernel_end;
off_t dtb_ram_start = 0;
size_t dtb_ram_size = 0;

bool_t dtb_node_get_rsv_ram (struct dtb_node *n) {
    if(strstr(n->name, "mmode_resv")) {
        addr_t start = n->reg.address;
        ulong_t len = n->reg.length;
        INFO_PRINT("Reseving %s region (%p, size %lu)\n", n->name, start, len);
        mm_boot_reserve_mem(start, len);
    }
    return true;
}

bool_t dtb_node_get_ram (struct dtb_node * n) {
    if(!strcmp(n->name, "memory")) {
        dtb_ram_size = n->reg.length;
        dtb_ram_start = n->reg.address;
        return false;
    }
    return true;
}

void
arch_reserve_boot_regions (unsigned long mbd)
{
    dtb_walk_devices(dtb_node_get_rsv_ram);
}

void
arch_detect_mem_map (mmap_info_t * mm_info,
                     mem_map_entry_t * memory_map,
                     ulong_t fdt)
{
    dtb_walk_devices(dtb_node_get_ram);

    if (dtb_ram_start == 0) {
      BMM_WARN("DTB did not contain memory segment. Assuming 128MB...\n");
      dtb_ram_size = 128000000;
      dtb_ram_start = (ulong_t) &kernel_end;
    }

    BMM_PRINT("Parsing RISC-V virt machine memory map\n");

    unsigned long long start, end;

    start = round_up(dtb_ram_start, PAGE_SIZE_4KB);
    end   = round_down(dtb_ram_start + dtb_ram_size, PAGE_SIZE_4KB);

    memory_map[0].addr = start;
    memory_map[0].len  = end-start;
    memory_map[0].type = MULTIBOOT_MEMORY_AVAILABLE;

    BMM_PRINT("Memory map[%d] - [%p - %p] sz:%llu <%s>\n",
        0,
        start,
        end,
		end - start,
        mem_region_types[memory_map[0].type]);
    mm_info->usable_ram += end-start;

    if (end > (mm_info->last_pfn << PAGE_SHIFT)) {
      mm_info->last_pfn = end >> PAGE_SHIFT;
    }

    mm_info->total_mem += end-start;

    ++mm_info->num_regions;
}
