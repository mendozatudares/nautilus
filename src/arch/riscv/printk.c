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

void panic(const char *fmt, ...);

// Print to the console. only understands %d, %x, %p, %s.
int printk(const char *fmt, ...)
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
    return 0;
}

void
panic(const char *fmt, ...)
{
    va_list args;
    printk("PANIC: ");
    va_start(args, fmt);
    printk(fmt, args);
    va_end(args);
    printk("\n");
    for(;;);
}

void
printk_init(void)
{
    spinlock_init(&printk_lock);
}

extern unsigned char _ctype[];
#define _D	0x04	/* digit */
#define _X	0x40	/* hex digit */
#define __ismask(x) (_ctype[(int)(unsigned char)(x)])
#define isdigit(c)	((__ismask(c)&(_D)) != 0)
#define isxdigit(c)	((__ismask(c)&(_D|_X)) != 0)
/* Works only for digits and letters, but small and fast */
#define TOLOWER(x) ((x) | 0x20)

static unsigned int simple_guess_base(const char *cp)
{
	if (cp[0] == '0') {
		if (TOLOWER(cp[1]) == 'x' && isxdigit(cp[2]))
			return 16;
		else
			return 8;
	} else {
		return 10;
	}
}

/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0;

	if (!base)
		base = simple_guess_base(cp);

	if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
		cp += 2;

	while (isxdigit(*cp)) {
		unsigned int value;

		value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
		if (value >= base)
			break;
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;
	return result;
}

/**
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long simple_strtol(const char *cp, char **endp, unsigned int base)
{
	if(*cp == '-')
		return -simple_strtoul(cp + 1, endp, base);
	return simple_strtoul(cp, endp, base);
}
