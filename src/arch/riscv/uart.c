#include <nautilus/cpu.h>
#include <nautilus/naut_types.h>
#include <nautilus/spinlock.h>
#include <arch/riscv/sbi.h>

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
#define Reg(reg) ((volatile unsigned char *)(0x10011000 + reg))

// the UART control registers.
// some have different meanings for
// read vs write.
// see http://byterunner.com/16550.html
#define RHR 0                 // receive holding register (for input bytes)
#define THR 0                 // transmit holding register (for output bytes)
#define IER 1                 // interrupt enable register
#define IER_RX_ENABLE (1<<0)
#define IER_TX_ENABLE (1<<1)
#define FCR 2                 // FIFO control register
#define FCR_FIFO_ENABLE (1<<0)
#define FCR_FIFO_CLEAR (3<<1) // clear the content of the two FIFOs
#define ISR 2                 // interrupt status register
#define LCR 3                 // line control register
#define LCR_EIGHT_BITS (3<<0)
#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate
#define LSR 5                 // line status register
#define LSR_RX_READY (1<<0)   // input is waiting to be read from RHR
#define LSR_TX_IDLE (1<<5)    // THR can accept another character to send

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

// the transmit output buffer.
spinlock_t uart_tx_lock;

#define UART_LOCK_CONF uint8_t _uart_tx_lock_flags
#define UART_LOCK() _uart_tx_lock_flags = spin_lock_irq_save(&uart_tx_lock)
#define UART_UNLOCK() spin_unlock_irq_restore(&uart_tx_lock, _uart_tx_lock_flags);

#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64_t uart_tx_w; // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
uint64_t uart_tx_r; // read next from uart_tx_buf[uar_tx_r % UART_TX_BUF_SIZE]

void uart_start();

void
uart_init(void)
{
    return;

    // disable interrupts.
    WriteReg(IER, 0x00);

    // special mode to set baud rate.
    WriteReg(LCR, LCR_BAUD_LATCH);

    // LSB for baud rate of 38.4K.
    WriteReg(0, 0x03);

    // MSB for baud rate of 38.4K.
    WriteReg(1, 0x00);

    // leave set-baud mode,
    // and set word length to 8 bits, no parity.
    WriteReg(LCR, LCR_EIGHT_BITS);

    // reset and enable FIFOs.
    WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

    // enable transmit and receive interrupts.
    WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

    spinlock_init(&uart_tx_lock);
}

// add a character to the output buffer and tell the
// UART to start sending if it isn't already.
// blocks if the output buffer is full.
// because it may block, it can't be called
// from interrupts; it's only suitable for use
// by write().
void
uart_putchar(int c)
{
    sbi_call(SBI_CONSOLE_PUTCHAR, c);
    /*
    UART_LOCK_CONF;

    while(1){
        UART_LOCK();
        if(uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE){
            // buffer is full.
            // wait for uartstart() to open up space in the buffer.
            UART_UNLOCK();
        } else {
            uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;
            uart_tx_w += 1;
            uart_start();
            UART_UNLOCK();
            return;
        }
    }
    */
}

// alternate version of uartputc() that doesn't 
// use interrupts, for use by kernel printf() and
// to echo characters. it spins waiting for the uart's
// output register to be empty.
void
uart_putchar_sync(int c)
{
    // wait for Transmit Holding Empty to be set in LSR.
    while((ReadReg(LSR) & LSR_TX_IDLE) == 0);
    WriteReg(THR, c);
}

void uart_puts(char *b) {
    while (b && *b) {
        uart_putchar(*b);
        b++;
    }
}

// if the UART is idle, and a character is waiting
// in the transmit buffer, send it.
// caller must hold uart_tx_lock.
// called from both the top- and bottom-half.
void
uart_start()
{
    while(1){
        if(uart_tx_w == uart_tx_r){
            // transmit buffer is empty.
            return;
        }
    
        if((ReadReg(LSR) & LSR_TX_IDLE) == 0){
            // the UART transmit holding register is full,
            // so we cannot give it another byte.
            // it will interrupt when it's ready for a new byte.
            return;
        }
    
        int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
        uart_tx_r += 1;
    
        WriteReg(THR, c);
    }
}

// read one input character from the UART.
// return -1 if none is waiting.
int
uart_getchar(void)
{
    sbi_call(SBI_CONSOLE_GETCHAR).value;
    // if(ReadReg(LSR) & 0x01){
    //     // input data is ready.
    //     return ReadReg(RHR);
    // } else {
    //     return -1;
    // }
}

// handle a uart interrupt, raised because input has
// arrived, or the uart is ready for more output, or
// both. called from trap.c.
void
uart_intr(void)
{
    UART_LOCK_CONF;

    // read and process incoming characters.
    while(1){
        int c = uart_getchar();
        if(c == -1) break;
        c = (c == '\r') ? '\n' : c;
        uart_putchar_sync(c);
    }

    // send buffered characters.
    UART_LOCK();
    uart_start();
    UART_UNLOCK();
}

/* Faking some vc stuff */

inline int nk_vc_is_active()
{
  return 0;
}

#include <nautilus/printk.h>
#include <stdarg.h>

int nk_vc_print(char *s)
{
    printk(s);
    return 0;
}

#define PRINT_MAX 1024

int nk_vc_printf(char *fmt, ...)
{
  char buf[PRINT_MAX];

  va_list args;
  int i;

  va_start(args, fmt);
  i=vsnprintf(buf,PRINT_MAX,fmt,args);
  va_end(args);
  nk_vc_print(buf);
  return i;
}

int nk_vc_log(char *fmt, ...)
{
  char buf[PRINT_MAX];

  va_list args;
  int i;
  
  va_start(args, fmt);
  i=vsnprintf(buf,PRINT_MAX,fmt,args);
  va_end(args);
  
  return i;
}
