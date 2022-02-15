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
#define UART_IP_RXWM      0x2

#define SIFIVE_DEFAULT_BAUD_RATE 115200

static struct sifive_serial_regs * regs;
static bool_t   inited = false;

int sifive_handler (excp_entry_t * excp, excp_vec_t vector, void *state) {
    panic("sifive_handler!\n");
    while (1) {
        uint32_t r = regs->rxfifo;
        if (r & UART_RXFIFO_EMPTY) break;
        char buf = r & 0xFF;
        // call virtual console here!
        serial_putchar(buf);
    }
}

int sifive_test(void) {
    arch_enable_ints();
    while(1) {
        printk("ie=%p, ip=%p, sie=%p, status=%p, sip=%p, pp=%p\t\n", regs->ie, regs->ip, read_csr(sie), read_csr(sstatus), read_csr(sip), plic_pending());
        asm volatile ("wfi");
        /* printk("%d\n", serial_getchar()); */
    }
}


static void sifive_init(addr_t addr, uint16_t irq) {
    printk("SIFIVE @ %p, irq=%d\n", addr, irq);
    regs = addr;

    regs->txctrl = UART_TXCTRL_TXEN;
    regs->rxctrl = UART_RXCTRL_RXEN;
    regs->ie = 0b10;

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
    /* dtb_walk_devices(dtb_node_get_sifive); */
    /* regs = (struct sifive_serial_regs *)0x10010000L; */
    sifive_init(0x10010000L, 4);
}

void serial_write(const char *b) {
    while (b && *b) {
        serial_putchar(*b);
        b++;
    }
}

void serial_putchar(unsigned char ch) {
    sbi_call(SBI_CONSOLE_PUTCHAR, ch);
}

int serial_getchar(void) {
    if (!inited) {
        struct sbiret ret = sbi_call(SBI_CONSOLE_GETCHAR);
        return ret.error == -1 ? -1 : ret.value;
    } else {
        uint32_t r = regs->rxfifo;
        return (r & UART_RXFIFO_EMPTY) ? -1 : r & 0xFF;
    }
}



