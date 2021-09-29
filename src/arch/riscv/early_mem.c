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

#include <arch/riscv/memlayout.h>

extern char * mem_region_types[6];

#ifndef NAUT_CONFIG_DEBUG_BOOTMEM
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define BMM_DEBUG(fmt, args...) DEBUG_PRINT("BOOTMEM: " fmt, ##args)
#define BMM_PRINT(fmt, args...) printk("BOOTMEM: " fmt, ##args)
#define BMM_WARN(fmt, args...)  WARN_PRINT("BOOTMEM: " fmt, ##args)


void 
arch_reserve_boot_regions (unsigned long mbd)
{

}

typedef struct {
  uint64_t addr;
  uint64_t len;
  uint32_t type; // same as for multiboot2
} __packed virt_mmap_entry_t;

/* From qemu's virt machine */
#define VIRT_MMAP_SIZE 13
virt_mmap_entry_t virt_mmap[VIRT_MMAP_SIZE] = {
/*  [VIRT_DEBUG] =  */    {        0x0,            0x100,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_MROM] =   */    {     0x1000,          0x11000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_TEST] =   */    {   0x100000,           0x1000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_RTC] =    */    {   0x101000,           0x1000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_CLINT] =  */    {  0x2000000,          0x10000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_PLIC] =   */    {  0xc000000,        0x4000000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_UART0] =  */    { 0x10000000,            0x100,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_VIRTIO] = */    { 0x10001000,           0x1000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_FLASH] =  */    { 0x20000000,        0x4000000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_DRAM] =   */    { 0x80000000, PHYSTOP-KERNBASE,   MULTIBOOT_MEMORY_AVAILABLE },
/*  [VIRT_PCIE_MMIO] = */ { 0x40000000,       0x40000000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_PCIE_PIO] =  */ { 0x03000000,       0x00010000,    MULTIBOOT_MEMORY_RESERVED },
/*  [VIRT_PCIE_ECAM] = */ { 0x30000000,       0x10000000,    MULTIBOOT_MEMORY_RESERVED },
};

void
arch_detect_mem_map (mmap_info_t * mm_info, 
                     mem_map_entry_t * memory_map,
                     unsigned long mbd)
{
    uint32_t i;

    BMM_PRINT("Parsing RISC-V virt machine memory map\n");

    for (i=0; i<VIRT_MMAP_SIZE; i++) {
    
    virt_mmap_entry_t *entry = &virt_mmap[i];
    ulong_t start,end;
    
    start = round_up(entry->addr, PAGE_SIZE_4KB);
    end   = round_down(entry->addr + entry->len, PAGE_SIZE_4KB);

    memory_map[i].addr = start;
    memory_map[i].len  = end-start;
    memory_map[i].type = entry->type;

    BMM_PRINT("Memory map[%d] - [%p - %p] <%s>\n", 
	      i, 
	      start,
	      end,
	      mem_region_types[memory_map[i].type]);
    
    if (entry->type == 1) {
      mm_info->usable_ram += entry->len;
    }

    if (end > (mm_info->last_pfn << PAGE_SHIFT)) {
      mm_info->last_pfn = end >> PAGE_SHIFT;
    }

    mm_info->total_mem += end-start;

    ++mm_info->num_regions;
    }
    
}


