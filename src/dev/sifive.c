#include <nautilus/nautilus.h>
#include <nautilus/devicetree.h>
#include <nautilus/irq.h>
#include <arch/riscv/sbi.h>
#include <dev/sifive.h>

#define UART_TXFIFO_FULL  0x80000000
#define UART_RXFIFO_EMPTY 0x80000000
#define UART_RXFIFO_DATA  0x000000ff
#define UART_TXCTRL_TXEN  0x1
#define UART_RXCTRL_RXEN  0x1

#define SIFIVE_DEFAULT_BAUD_RATE 115200

static addr_t   base = 0;
static uint64_t clock = 0;
static bool_t   inited = false;

static inline struct sifive_serial_regs * regs(void) {
    return (struct sifive_serial_regs *)base;
}

int sifive_handler (excp_entry_t * excp, excp_vec_t vector, void *state) {
    panic("sifive_handler!\n");
    while (1) {
        union rx_data r;
        r.val = regs()->rxdata.val;
        if (r.isEmpty) { break; }
        char buf = r.data;
        printk("%c", buf);
        // call virtual console here!
    }
}

int sifive_test(void) {
    arch_enable_ints();
    while(1) {
        printk("ie=%p, ip=%p, sie=%p, status=%p, sip=%p, pp=%p\t", regs()->ie, regs()->ip, read_csr(sie), read_csr(sstatus), read_csr(sip), plic_pending());
        printk("%d\n", serial_getchar());
        asm volatile ("wfi");
    }
}


static void sifive_init(addr_t addr, uint16_t irq) {
    printk("SIFIVE @ %p, irq=%d\n", addr, irq);
    base = addr;

    /* uart_clock = clock; */
    /* uint32_t baud = SIFIVE_DEFAULT_BAUD_RATE; */
    /* uint64_t quotient = (clock + baud - 1) / baud; */

    /* regs(base)->div = (quotient == 0) ? 0 : (uint32_t)(quotient - 1); */

    regs()->txctrl.enable = 1;
    regs()->rxctrl.enable = 1;
    regs()->ie = 2;

    inited = true;

    arch_irq_install(irq, sifive_handler);
}

bool_t dtb_node_sifive_compatible(struct dtb_node *n) {
    for (off_t i = 0; i < n->ncompat; i++) {
        if (strstr(n->compatible[i], "sifive,uart0")) {
            return true;
        }
    }
    return false;
}

bool_t dtb_node_get_sifive(struct dtb_node *n) {
    if (dtb_node_sifive_compatible(n)) {
        sifive_init(n->address, n->irq);
        return false;
    }
    return true;
}

void serial_init(void) {
    dtb_walk_devices(dtb_node_get_sifive);
}

void serial_write(const char *b) {
    while (b && *b) {
        serial_putchar(*b);
        b++;
    }
}

void serial_putchar(unsigned char ch) {
    /* if (!inited) { */
        sbi_call(SBI_CONSOLE_PUTCHAR, ch);
    /* } else { */
    /*     while (regs()->txdata.isFull); */
    /*     regs()->txdata.val = ch; */
    /* } */
}

int serial_getchar(void) {
    if (!inited) {
        struct sbiret ret = sbi_call(SBI_CONSOLE_GETCHAR);
        return ret.error == -1 ? -1 : ret.value;
    } else {
        union rx_data data;
        data.val = regs()->rxdata.val;
        return data.isEmpty ? -1 : data.data;
    }
}



