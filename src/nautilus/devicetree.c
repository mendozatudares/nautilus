#include <nautilus/devicetree.h>
#include <nautilus/naut_types.h>
#include <nautilus/list.h>
#include <nautilus/printk.h>
#include <nautilus/mm.h>

//! Byte swap int
static uint32_t bswap32(uint64_t val) {
  val =  (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
  return (val & 0x00FF00FF) << 8  | (val & 0xFF00FF00) >> 8;
}

static uint64_t bswap64(uint64_t val) {
  val =  (val & 0x00000000FFFFFFFF) << 32 | (val & 0xFFFFFFFF00000000) >> 32;
  val =  (val & 0x0000FFFF0000FFFF) << 16 | (val & 0xFFFF0000FFFF0000) >> 16;
  return (val & 0x00FF00FF00FF00FF) << 8  | (val & 0xFF00FF00FF00FF00) >> 8;
}

int dtb_node_get_addr_cells(struct dtb_node *n) {
  if (n->address_cells != -1) return n->address_cells;
  if (n->parent != NULL) return dtb_node_get_addr_cells(n->parent);
  return 0;
}

int dtb_node_get_size_cells(struct dtb_node *n) {
  if (n->size_cells != -1) return n->size_cells;
  if (n->parent != NULL) return dtb_node_get_size_cells(n->parent);
  return 0;
}

struct prop* device_tree_node_get_prop(struct device_tree_node *n, char* name) {
  struct prop *tmp_prop;
  list_for_each_entry(tmp_prop, &n->props, node) {
    if (strcmp(tmp_prop->name, name) == 0)
      return tmp_prop;
  };
  return 0;
}

void device_tree_node_set_prop(struct device_tree_node *n, char* name, int vlen, void *value) {
  struct prop* p = device_tree_node_get_prop(n, name);
  if (p == 0) {
    p = malloc(sizeof(*p));
    p->name = name;
    p->vlen = vlen;
  }

  p->values = malloc(sizeof(uint8_t) * vlen);
  for (int i = 0; i < vlen; i++) {
    p->values[i] = ((uint8_t *)value)[i];
  }
}

struct device_tree_node *device_tree_node_spawn(struct device_tree_node *n, char *name) {
  struct device_tree_node *nn = malloc(sizeof(*nn));
  nn->name = name;
  nn->parent = n;

  struct child *c = malloc(sizeof(*c));
  list_add_tail(&c->node, &n->children);
  return n;
}

void device_tree_dump(struct device_tree *dt) { 
  device_tree_node_dump(&dt->root, 0);
}

#define round_up(x, y) (((x) + (y)-1) & ~((y)-1))

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

#define FDT_T_STRING 0
#define FDT_T_INTARR 1
#define FDT_T_INT 2
#define FDT_T_EMPTY 4

#define FDT_NUM_NODES 64

static struct dtb_fdt_header *global_fdt_header = NULL;
static struct dtb_node devices[FDT_NUM_NODES];
static int next_device = 0;

static struct dtb_node *alloc_device(const char *name) {
  struct dtb_node *dev = &devices[next_device++];
  memset(dev, 0, sizeof(*dev));
  strncpy(dev->name, name, 32);
  dev->is_device = false;
  dev->address_cells = dev->size_cells = -1;

  int at_off = -1;
  for (int i = 0; true; i++) {
    if (name[i] == 0) {
      /* reached the end, didn't find an @ */
      break;
    }

    if (name[i] == '@') {
      at_off = i;
      break;
    }
  }

  if (at_off != -1) {
    if (sscanf(name + at_off + 1, "%lx", &dev->address) != 1) {
      // printk("INVALID NODE NAME: '%s'\n", name);
    }
    dev->name[at_off] = 0;
  }
  return dev;
}

#define STREQ(s1, s2) (!strcmp((s1), (s2)))

static void node_set_prop(struct dtb_node *node, const char *name, int len, uint8_t *val) {
  // printk("%s.%s\n", node->name, name);
  if (STREQ(name, "#address-cells")) {
    node->address_cells = bswap32(*(uint32_t *)val);
    return;
  }

  if (STREQ(name, "#size-cells")) {
    node->size_cells = bswap32(*(uint32_t *)val);
    return;
  }

  if (STREQ(name, "interrupts")) {
    int size_cells = dtb_node_get_size_cells(node);
    node->irq = bswap32(*(uint32_t *)val);
    return;
  }

  if (STREQ(name, "reg")) {
    node->is_device = true;
    /* TODO: It's unsafe to assume 64 bit here... But since we are 64bit only... (for now) */
    // int *cells = (unsigned long *)val;

    int addr_cells = dtb_node_get_addr_cells(node);
    int size_cells = dtb_node_get_size_cells(node);

    // printk("%s: addr %d, size %d\n", node->name, addr_cells, size_cells);
    if (addr_cells > 0) {
      if (addr_cells == 1) node->reg.address = bswap32(*(uint32_t *)val);
      if (addr_cells == 2) node->reg.address = bswap64(*(uint64_t *)val);
    }
    val += addr_cells * 4;

    if (size_cells > 0) {
      if (size_cells == 1) node->reg.length = bswap32(*(uint32_t *)val);
      if (size_cells == 2) node->reg.length = bswap64(*(uint64_t *)val);
    }
    val += size_cells * 4;

    return;
  }

  if (STREQ(name, "compatible")) {
    node->is_device = true;
    strncpy(node->compatible, (const char *)val, sizeof(node->compatible));
    // printk("compatible: %s\n", val);

    return;
  }
  // printk("   %s@%llx\t%s = %p\n", node->name, node->address, name, val);
}


void dtb_walk_devices(bool (*callback)(struct dtb_node *)) {
  for (int i = 0; i < next_device; i++) {
    if (devices[i].is_device) {
      if (!callback(&devices[i])) break;
    }
  }
}

static void spaces(int n) {
  for (int i = 0; i < n; i++)
    printk("  ");
}

void dump_dtb(struct dtb_node *node, int depth) {
  spaces(depth);
  printk("%s@%llx\n", node->name, node->address);
  if (node->address_cells != -1) {
    spaces(depth);
    printk("- #address-cells: %d\n", node->address_cells);
  }
  if (node->size_cells != -1) {
    spaces(depth);
    printk("- #size-cells: %d\n", node->size_cells);
  }

  spaces(depth);
  printk("- compatible: %s\n", node->compatible);


  if (node->is_device) {
    spaces(depth);
    printk("- reg: %p %d\n", node->reg.address, node->reg.length);
  }
  for (struct dtb_node *cur = node->children; cur != NULL; cur = cur->sibling) {
    dump_dtb(cur, depth + 1);
  }
}

int dtb_parse(struct dtb_fdt_header *fdt) {
  printk("fdt at %llx\n", fdt);
  global_fdt_header = fdt;

  printk("  magic: %p\n", bswap32(fdt->magic));
  printk("  totalsize: %d\n", bswap32(fdt->totalsize));
  printk("  off_dt_struct: %p\n", bswap32(fdt->off_dt_struct));
  printk("  off_dt_strings: %p\n", bswap32(fdt->off_dt_strings));
  printk("  off_mem_rsvmap: %p\n", bswap32(fdt->off_mem_rsvmap));
  printk("  version: %d\n", bswap32(fdt->version));
  printk("  last_comp_version: %d\n", bswap32(fdt->last_comp_version));
  printk("  boot_cpuid_phys: %p\n", bswap32(fdt->boot_cpuid_phys));
  printk("  size_dt_strings: %p\n", bswap32(fdt->size_dt_strings));
  printk("  size_dt_struct: %p\n", bswap32(fdt->size_dt_struct));

	next_device = 0;

  if (next_device != 0) panic("expected next_device = 0, got %d\n", next_device);
  struct dtb_node *root = alloc_device("");

  struct dtb_node *node = root;
  struct dtb_node *new_node = NULL;

  uint32_t *sp = (uint32_t *)((off_t)fdt + bswap32(fdt->off_dt_struct));
  const char *strings = (char *)((off_t)fdt + bswap32(fdt->off_dt_strings));
  int depth = 0;

  while (bswap32(*sp) != FDT_END) {
    uint32_t op = bswap32(*sp);
    /* sp points to the next word */
    sp++;

    uint32_t len;
    uint32_t nameoff;
    char *name;
    char *valptr = NULL;
    char value[256];

    switch (op) {
      case FDT_BEGIN_NODE:
        name = (char *)sp;

        len = round_up(strlen(name) + 1, 4) / 4;
        for (int i = 0; i < len; i++)
          sp++;
        new_node = alloc_device(name);

        new_node->parent = node;

        new_node->sibling = node->children;
        node->children = new_node;

        node = new_node;
        new_node = NULL;

        depth++;

        break;
      case FDT_END_NODE:
        depth--;
        node = node->parent;
        break;

      case FDT_PROP:
        len = bswap32(*sp);
        sp++;
        nameoff = bswap32(*sp);
        sp++;
        valptr = (uint8_t *)sp;
        node_set_prop(node, strings + nameoff, len, valptr);
        for (int i = 0; i < round_up(len, 4) / 4; i++)
          sp++;

        break;

      case FDT_NOP:
        // printk("nop\n");
        break;

      case FDT_END:
        // printk("end\n");
        break;
    }
  }

  dump_dtb(node, 0);
  return next_device;
}

static struct fdt_type {
  char *name;
  int type;
} fdt_types[] = {
    {"compatible", FDT_T_STRING},
    {"model", FDT_T_STRING},
    {"phandle", FDT_T_INT},
    {"status", FDT_T_STRING},
    {"#address-cells", FDT_T_INT},
    {"#size-cells", FDT_T_INT},
    {"reg", FDT_T_INT},
    {"virtual-reg", FDT_T_INT},
    {"ranges", FDT_T_EMPTY},
    {"dma-ranges", FDT_T_EMPTY}, /* TODO: <prop-encoded-array> */
    {"name", FDT_T_STRING},
    {"device_type", FDT_T_STRING},
    {0, 0},
};

static int get_fdt_prop_type(const char *c) {
  struct fdt_type *m = &fdt_types[0];

  while (m->name != NULL) {
    if (!strcmp(c, m->name)) return m->type;
    m++;
  }
  return -1;
}

void device_tree_init(struct device_tree *dt, struct dtb_fdt_header *fdt) {
  dt->fdt = (struct dtb_fdt_header *)malloc(bswap32(fdt->totalsize));
  memcpy(fdt, dt->fdt, bswap32(fdt->totalsize));

  uint32_t *sp = (uint32_t *)((off_t)fdt + bswap32(fdt->off_dt_struct));
  char *strings = (char *)((off_t)fdt + bswap32(fdt->off_dt_strings));
  int depth = 0;

  struct device_tree_node *node = &dt->root;

  while (bswap32(*sp) != FDT_END) {
    uint32_t op = bswap32(*sp);
    /* sp points to the next word */
    sp++;

    uint32_t len;
    uint32_t nameoff;
    char *name;
    char *valptr = NULL;
    char value[256];

    switch (op) {
      case FDT_BEGIN_NODE:
        name = (char *)sp;

        len = round_up(strlen(name) + 1, 4) / 4;
        for (int i = 0; i < len; i++)
          sp++;
        device_tree_node_spawn(node, name);
        depth++;

        break;
      case FDT_END_NODE:
        depth--;
        node = node->parent;
        break;

      case FDT_PROP:
        len = bswap32(*sp);
        sp++;
        nameoff = bswap32(*sp);
        sp++;
        valptr = (char *)sp;
        device_tree_node_set_prop(node, strings + nameoff, len, (uint8_t *)valptr);

        for (int i = 0; i < round_up(len, 4) / 4; i++)
          sp++;

        break;

      case FDT_NOP:
        // printk("nop\n");
        break;

      case FDT_END:
        // printk("end\n");
        break;
    }
  }
}


void device_tree_deinit(struct device_tree *dt) { mm_boot_free((void *)dt->fdt, sizeof(dt->fdt)); }

void device_tree_node_dump(struct device_tree_node *n, int depth) {
  for (int i = 0; i < depth; i++)
    printk("    ");
  printk("%s {\n", n->name);

  struct prop *tmp_prop;
  list_for_each_entry(tmp_prop, &n->props, node) {
    for (int i = 0; i < depth + 1; i++)
      printk("    ");

    void *raw = (void *)tmp_prop->values;
    int len = tmp_prop->vlen;
    printk("len = %d\n", len);

    printk("%s", tmp_prop->name);
    switch (get_fdt_prop_type(tmp_prop->name)) {
      case FDT_T_STRING:
        printk(" = '%s'", (const char *)raw);
        break;

      case FDT_T_INTARR:
        for (int i = 0; i < len / 4; i++) {
          printk(" = <0x%x>", *((uint32_t *)raw) + i);
        }
        break;
      case FDT_T_INT:
        printk(" = <0x%x>", *((uint32_t *)raw));
        break;

      case FDT_T_EMPTY:
        break;
      default:
        printk(" = ?");
        break;
    };

    printk("\n");
  };

  struct child *tmp_child;
  list_for_each_entry(tmp_child, &n->children, node) {
    device_tree_node_dump(tmp_child->dt_node, depth + 1);
  };

  for (int i = 0; i < depth; i++)
    printk("    ");
  printk("};\n\n");
}
