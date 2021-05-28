//
// formatted console output -- printk, panic.
//

#include <stdarg.h>

#include <nautilus/naut_types.h>
#include <nautilus/spinlock.h>

// lock to avoid interleaving concurrent printk's.
spinlock_t printk_lock;

#define PRINTK_LOCK_CONF uint8_t _printk_lock_flags
#define PRINTK_LOCK() _printk_lock_flags = spin_lock_irq_save(&printk_lock)
#define PRINTK_UNLOCK() spin_unlock_irq_restore(&printk_lock, _printk_lock_flags);

static char digits[] = "0123456789abcdef";

void uart_putchar(int c);
void uart_puts(char *b);

static void
print_int(int xx, int base, int sign)
{
    char buf[16];
    int i;
    uint32_t x;

    if(sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);

    if(sign)
        buf[i++] = '-';

    while(--i >= 0)
        uart_putchar(buf[i]);
}

static void
print_ptr(uint64_t x)
{
  int i;
  uart_putchar('0');
  uart_putchar('x');
  for (i = 0; i < (sizeof(uint64_t) * 2); i++, x <<= 4)
    uart_putchar(digits[x >> (sizeof(uint64_t) * 8 - 4)]);
}

void panic(char *s);

// Print to the console. only understands %d, %x, %p, %s.
void
printk(char *fmt, ...)
{
    PRINTK_LOCK_CONF;
    va_list ap;
    int i, c;
    char *s;

    PRINTK_LOCK();

    if (fmt == 0)
        panic("null fmt");

    va_start(ap, fmt);
    for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
        if(c != '%'){
            uart_putchar(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if(c == 0)
            break;
        switch(c) {
            case 'd':
                print_int(va_arg(ap, int), 10, 1);
                break;
            case 'x':
                print_int(va_arg(ap, int), 16, 1);
                break;
            case 'p':
                print_ptr(va_arg(ap, uint64_t));
                break;
            case 's':
                if((s = va_arg(ap, char*)) == 0)
                    s = "(null)";
                for(; *s; s++)
                    uart_putchar(*s);
                break;
            case '%':
                uart_putchar('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                uart_putchar('%');
                uart_putchar(c);
                break;
        }
    }

    PRINTK_UNLOCK();
}

void
panic(char *s)
{
    printk("PANIC: ");
    printk(s);
    printk("\n");
    for(;;);
}

void
printk_init(void)
{
    spinlock_init(&printk_lock);
}
