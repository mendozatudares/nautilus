#ifndef __DEVICETREE_H__
#define __DEVICETREE_H__

#include <nautilus/naut_types.h>
#include <nautilus/list.h>
#include <nautilus/mm.h>

struct dtb_fdt_header {
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;
  uint32_t size_dt_struct;
};

struct dtb_reg {
  off_t address;
  size_t length;
};

#define DTB_MAX_COMPATIBLE 8

struct dtb_node {
  // name@address
  char name[32];
  off_t address;
  struct dtb_node *parent;

  int ncompat;
  char *compatible[DTB_MAX_COMPATIBLE];
  char compat[128];
  bool_t is_device;

  short address_cells;
  short size_cells;
  short irq;

  off_t fdt_offset;

  struct dtb_reg reg;

  /* The list of children */
  struct dtb_node *children; /* A pointer to the start of a sibling list */
  struct dtb_node *sibling;  /* A pointer to the next sibling of the parent's children */
};

static inline int dtb_node_get_addr_cells(struct dtb_node *n) {
  if (n->address_cells != -1) return n->address_cells;
  if (n->parent != NULL) return dtb_node_get_addr_cells(n->parent);
  return 0;
}

static inline int dtb_node_get_size_cells(struct dtb_node *n) {
  if (n->size_cells != -1) return n->size_cells;
  if (n->parent != NULL) return dtb_node_get_size_cells(n->parent);
  return 0;
}

/* Return the number of devices nodes found */
int dtb_parse(struct dtb_fdt_header *hdr);
/* Walk the devices with a callback. Continue if the callback returns true */
void dtb_walk_devices(bool_t (*callback)(struct dtb_node *));

/*
  * Some of the standard props
  * source:
  * https://devicetree-specification.readthedocs.io/en/v0.2/devicetree-basics.html#sect-property-values
  */
struct prop {
  char* name;
  uint8_t* values;
  int vlen;

  struct list_head node;
};

struct child {
  struct device_tree_node *dt_node;

  struct list_head node;
};

struct device_tree_node {
  char* name;
  struct device_tree_node *parent;

  struct list_head props;


  struct list_head children;
};

struct device_tree {
  struct dtb_fdt_header *fdt;
  struct device_tree_node root;
};

struct device_tree_node *device_tree_node_spawn(struct device_tree_node *n, char *name);
struct prop* device_tree_node_get_prop(struct device_tree_node *n, char* name);
void device_tree_node_set_prop(struct device_tree_node *n, char* name, int vlen, void *value);
void device_tree_node_dump(struct device_tree_node *n, int depth);

void device_tree_init(struct device_tree *dt, struct dtb_fdt_header *fdt);
void device_tree_deinit(struct device_tree *dt);
void device_tree_dump(struct device_tree *dt);

#endif
