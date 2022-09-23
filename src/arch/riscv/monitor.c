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
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2020, Drew Kersnar <drewkersnar2021@u.northwestern.edu>
 * Copyright (c) 2020, The Interweaving Project <http://interweaving.org>
 *                     The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Drew Kersnar <drewkersnar2021@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <arch/riscv/sbi.h>
#include <dev/sifive.h>


#define ITERATIONS 100
#define NUM_READINGS 100.0

#define read_csr(name)                         \
  ({                                           \
    uint64_t x;                             \
    asm volatile("csrr %0, " #name : "=r"(x)); \
    x;                                         \
  })

#define write_csr(csr, val)                                             \
  ({                                                                    \
    unsigned long __v = (unsigned long)(val);                           \
    __asm__ __volatile__("csrw " #csr ", %0" : : "rK"(__v) : "memory"); \
  })

static inline uint64_t __attribute__((always_inline))
rdtsc (void)
{
    return read_csr(time);
}

static char * long_to_string(long x)
{
  static char buf[20];
  for(int i=0; i<19; i++){
    buf[i] = '0';
  }
  for(int i=18; x>0; i--)
  {
    buf[i] = (char) ((x%10) + 48);
    x = x/10;
  }
  buf[19] = 0;
  return buf;
}

static void outb (unsigned char val, uint64_t addr)
{
    asm volatile ("sb  %[_v], 0(%[_a])":: [_a] "r" (addr), [_v] "r" (val));
}

static uint8_t inb (uint64_t addr)
{
    uint8_t ret;
    asm volatile ("lb  %[_r], 0(%[_a])": [_r] "=r" (ret): [_a] "r" (addr));
    return ret;
}

static void my_memset(void *b,uint8_t val, uint32_t count){
	uint32_t i;
	for(i=0;i<count;i++){
	  ((uint8_t*)b)[i]=val;
	}
}

static void my_memcpy(void *b,void *s, uint32_t count){
        uint32_t i;
        for(i=0;i<count;i++){
	  ((uint8_t*)b)[i]=((uint8_t*)s)[i];
        }
}

static int my_strlen(char *b)
{
  int count=0;

  while (*b++) { count++; }
  return count;
}

int my_strcmp (const char * s1, const char * s2)
{
    while (1) {
    int cmp = (*s1 - *s2);

    if ((cmp != 0) || (*s1 == '\0') || (*s2 == '\0')) {
        return cmp;
    }

    ++s1;
    ++s2;
    }
}

#define DB(x) serial_putchar(x)
#define DHN(x) serial_putchar(((x & 0xF) >= 10) ? (((x & 0xF) - 10) + 'a') : ((x & 0xF) + '0'))
#define DHB(x) DHN(x >> 4) ; DHN(x);
#define DHW(x) DHB(x >> 8) ; DHB(x);
#define DHL(x) DHW(x >> 16) ; DHW(x);
#define DHQ(x) DHL(x >> 32) ; DHL(x);
#define DS(x) { char *__curr = x; while(*__curr) { DB(*__curr); __curr++; } }

static void print(char *b)
{
    while (b && *b) {
        serial_putchar(*b);
        b++;
    }
    serial_putchar('\n');
}

// Keyboard stuff repeats here to be self-contained

// Keys and scancodes are represented internally as 16 bits
// so we can indicate nonexistence (via -1)
//
// A "keycode" consists of the translated key (lower 8 bits)
// combined with an upper byte that reflects status of the
// different modifier keys
typedef uint16_t nk_keycode_t;
typedef uint16_t nk_scancode_t;

// Special tags to indicate unavailabilty
#define NO_KEY      ((nk_keycode_t)(0xffff))
#define NO_SCANCODE ((nk_scancode_t)(0xffff))

// Special ascii characters
#define ASCII_ESC 0x1B
#define ASCII_BS  0x08

// writes a line of text into a buffer
static void wait_for_command(char *buf, int buffer_size)
{

  int key;
  int curr = 0;
  while (1)
  {


		struct sbiret ret = sbi_call(SBI_CONSOLE_GETCHAR);
		key = ret.error;

		/* printk("e: %d, v: %d\n", ret.error, ret.value); */

		// continue;

    /* key = serial_getchar(); */
    /* printk("%c", key); */
    // key = ps2_wait_for_key();
    // uint16_t key_encoded = kbd_translate_monitor(key);
    //vga_clear_screen(vga_make_entry(key, vga_make_color(COLOR_FOREGROUND, COLOR_BACKGROUND)));

    if (key == '\r')
    {
      // return completed command
      buf[curr] = 0;
      return;
    }

    if (key != -1) {
      char key_char = (char)key;

      if (curr < buffer_size - 1)
      {
        DB(key_char);
        buf[curr++] = key_char;
      }
      else
      {
        // they can't type any more
        buf[curr] = 0;
        return;
      }

      //strncat(*buf, &key_char, 1);
      //printk(buf);
    }
  };
}

static int is_hex_addr(char *addr_as_str)
{
  int str_len = my_strlen(addr_as_str);

  // 64 bit addresses can't be larger than 16 hex digits, 18 including 0x
  if(str_len > 18) {
    return 0;
  }

  if((addr_as_str[0] != '0') || (addr_as_str[1] != 'x')) {
    return 0;
  }

  for(int i = 2; i < str_len; i++) {
    char curr = addr_as_str[i];
    if((curr >= '0' && curr <= '9')
      || (curr >= 'A' && curr <= 'F')
      || (curr >= 'a' && curr <= 'f'))  {
      continue;
    } else {
      return 0;
    }
  }
  return 1;
}

static uint64_t get_hex_addr(char *addr_as_str)
{

  uint64_t addr = 0;
  int power_of_16 = 1;
  // iterate backwards from the end of the address to the beginning, stopping before 0x
  for(int i = (my_strlen(addr_as_str) - 1); i > 1; i--) {
    char curr = addr_as_str[i];
    if(curr >= '0' && curr <= '9')  {
      addr += (curr - '0')*power_of_16;
    }
    else if(curr >= 'A' && curr <= 'F') {
      addr += (curr - 'A' + 10)*power_of_16;
    }
    else if(curr >= 'a' && curr <= 'f') {
      addr += (curr - 'a' + 10)*power_of_16;
    } else {
      // something broken
      //ASSERT(false);
    }

    power_of_16*=16;
  }
  return addr;

}


static int is_dec_addr(char *addr_as_str)
{
  int str_len = my_strlen(addr_as_str);

  // // 64 bit addresses can't be larger than 16 hex digits, 18 including 0x
  // if(str_len > 18) {
  //   return 0;
  // }
  // TODO: check if dec_addr too big

  for(int i = 0; i < str_len; i++) {
    char curr = addr_as_str[i];
    if((curr >= '0' && curr <= '9'))  {
      continue;
    } else {
      return 0;
    }
  }
  return 1;
}

static uint64_t get_dec_addr(char *addr_as_str)
{
  uint64_t addr = 0;
  int power_of_10 = 1;
  // iterate backwards from the end of the address to the beginning, stopping before 0x
  for(int i = (my_strlen(addr_as_str) - 1); i >= 0; i--) {
    char curr = addr_as_str[i];
    if(curr >= '0' && curr <= '9')  {
      addr += (curr - '0')*power_of_10;
    } else {
      // something broken
      //ASSERT(false);
    }

    power_of_10*=10;
  }
  return addr;

}

static int is_dr_num(char *addr_as_str)
{
  int str_len = my_strlen(addr_as_str);

  // TODO: check if dec_addr too big

  for(int i = 0; i < str_len; i++) {
    char curr = addr_as_str[i];
    if((curr >= '0' && curr <= '3'))  {
      continue;
    } else {
      return 0;
    }
  }
  return 1;
}

static uint64_t get_dr_num(char *num_as_str)
{

  uint64_t num = 0;
  int power_of_10 = 1;
  // iterate backwards from the end of the address to the beginning, stopping before 0x
  for(int i = (my_strlen(num_as_str) - 1); i >= 0; i--) {
    char curr = num_as_str[i];
    if(curr >= '0' && curr <= '9')  {
      num += (curr - '0')*power_of_10;
    } else {
      // something broken
      //ASSERT(false);
    }

    power_of_10*=10;
  }
  return num;

}

// Private tokenizer since we may want to use a non-reentrant version at
// some point.., and also because we don't want to rely on the other
// copy in naut_string, which looks lua-specific
//

static char *
__strtok_r_monitor(char *s, const char *delim, char **last)
{
	char *spanp, *tok;
	int c, sc;

	if (s == NULL && (s = *last) == NULL)
		return (NULL);

	/*
	 * 	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 * 	 	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * 	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * 	 	 * Note that delim must have one NUL; we stop if we see that, too.
	 * 	 	 	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = '\0';
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

static char *
strtok_monitor(char *s, const char *delim)
{
	static char *last;

	return (__strtok_r_monitor(s, delim, &last));
}

static char *
get_next_word(char *s)
{
  const char delim[3] = " \t";
  return strtok_monitor(s, delim);
}

// Executing user commands

static int execute_quit(char command[])
{
  print("quit executed");
  return 1;
}

static int execute_help(char command[])
{
  print("commands:");
  print("  quit");
  print("  help");
  print("  paging");
  print("  pf");
  print("  test");
  print("  pwrstats");
  print("  threading");
  return 0;
}

static inline void
tlb_flush (void)
{
    asm volatile("sfence.vma zero, zero");
}

extern uint64_t nk_paging_default_satp();
static void paging_on(void)
{
  write_csr(satp, nk_paging_default_satp());
  tlb_flush();
}

static void paging_off(void)
{
  write_csr(satp, 0);
  tlb_flush();
}

static int execute_paging(char command[])
{
  if (!read_csr(satp)) {
    paging_on();
    print("paging is on now\n\r");
  } else {
    paging_off();
    print("paging if off now\n\r");
  }
  return 0;
}

#define PTE_V (1L << 0) // valid
#define PTE2PA(pte) (((pte) >> 10) << 12)

static int execute_pf(char command[])
{
  print("executing test\n\r");
  uint64_t * page_table = (uint64_t *)(nk_paging_default_satp() << 12);
  page_table[3] &= ~PTE_V;
  volatile uint64_t x = *((uint64_t *)PTE2PA(page_table[3]));
  print("test executed successfully\n\r");
  return 0;
}

static int execute_rapl(char command[])
{
  // pwrstat_init();
  return 0;
}


static long low_locality()   // 128 accesses, all from different pages
{
  // flush TLB

  tlb_flush();

  unsigned int *x = (unsigned int *)0xC0000000U;
  unsigned long sum = 0;
  for (int i=0; i<ITERATIONS; i++)
  {
    for (unsigned int j=0; j<0x8000000; j+=0x100000)
    {
      sum += x[j];
    }
  }
  return sum;
}
static long medium_locality()   // 128 accesses, 8 from same page
{
  // flush TLB
 tlb_flush();

  unsigned int *x = (unsigned int *)0xC0000000U;
  unsigned long sum = 0;
  for (int i=0; i<ITERATIONS; i++)
  {
    for (unsigned int j=0; j<0x1000000; j+=0x20000)
    {
      sum += x[j];
    }
  }
  return sum;
}
static long high_locality()   // 128 accesses, all from same page
{
  // flush TLB

  tlb_flush();

  unsigned int *x = (unsigned int *)0xC0000000U;
  unsigned long sum = 0;
  for (int i=0; i<ITERATIONS; i++)
  {
    for (unsigned int j=0; j<128; j+=1)
    {
      sum += x[j];
    }
  }
  return sum;
}

static int execute_test(char command[])
{
  double avg_cycles = 0;
  unsigned long avg_sum = 0;

  print("========================== PAGING OFF ==========================\n\r");
  paging_off();
  for (int i=0; i<NUM_READINGS; i++)
  {
    unsigned long t1 = rdtsc();
    unsigned long temp = high_locality();
    unsigned long t2 = rdtsc();
    avg_cycles += (t2-t1);
    avg_sum += temp;
  }
  if(avg_sum){
    print(long_to_string(avg_sum));
    print("\n\r");
  }
  print("avg cycles with high locality: ");
  print(long_to_string((unsigned long)(avg_cycles/NUM_READINGS)));
  print("\n\r");

  avg_cycles = 0;
  avg_sum = 0;
  for (int i=0; i<NUM_READINGS; i++)
  {
    unsigned long t1 = rdtsc();
    unsigned long temp = medium_locality();
    unsigned long t2 = rdtsc();
    avg_cycles += (t2-t1);
    avg_sum += temp;
  }
  if(avg_sum){
    print(long_to_string(avg_sum));
    print("\n\r");
  }
  print("avg cycles with medium locality: ");
  print(long_to_string((unsigned long)(avg_cycles/NUM_READINGS)));
  print("\n\r");

  avg_cycles = 0;
  avg_sum = 0;
  for (int i=0; i<NUM_READINGS; i++)
  {
    unsigned long t1 = rdtsc();
    unsigned long temp = low_locality();
    unsigned long t2 = rdtsc();
    avg_cycles += (t2-t1);
    avg_sum += temp;
  }
  if(avg_sum){
    print(long_to_string(avg_sum));
    print("\n\r");
  }
  print("avg cycles with low locality: ");
  print(long_to_string((unsigned long)(avg_cycles/NUM_READINGS)));
  print("\n\n\n\r");

  print("========================== PAGING ON ==========================\n\r");
  paging_on();

  avg_cycles = 0;
  avg_sum = 0;
  for (int i=0; i<NUM_READINGS; i++)
  {
    unsigned long t1 = rdtsc();
    unsigned long temp = high_locality();
    unsigned long t2 = rdtsc();
    avg_cycles += (t2-t1);
    avg_sum += temp;
  }
  if(avg_sum){
    print(long_to_string(avg_sum));
    print("\n\r");
  }
  print("avg cycles with high locality: ");
  print(long_to_string((unsigned long)(avg_cycles/NUM_READINGS)));
  print("\n\r");

  avg_cycles = 0;
  avg_sum = 0;
  for (int i=0; i<NUM_READINGS; i++)
  {
    unsigned long t1 = rdtsc();
    unsigned long temp = medium_locality();
    unsigned long t2 = rdtsc();
    avg_cycles += (t2-t1);
    avg_sum += temp;
  }
  if(avg_sum){
    print(long_to_string(avg_sum));
    print("\n\r");
  }
  print("avg cycles with medium locality: ");
  print(long_to_string((unsigned long)(avg_cycles/NUM_READINGS)));
  print("\n\r");

  avg_cycles = 0;
  avg_sum = 0;
  for (int i=0; i<NUM_READINGS; i++)
  {
    unsigned long t1 = rdtsc();
    unsigned long temp = low_locality();
    unsigned long t2 = rdtsc();
    avg_cycles += (t2-t1);
    avg_sum += temp;
  }
  if(avg_sum){
    print(long_to_string(avg_sum));
    print("\n\r");
  }
  print("avg cycles with low locality: ");
  print(long_to_string((unsigned long)(avg_cycles/NUM_READINGS)));
  print("\n\r");
  return 0;
}

extern int execute_threading(char command[]);

static int execute_potential_command(char command[])
{
  int quit = 0;
  char* print_string = "";

  char* word = get_next_word(command);
	if (word == NULL) return 0;

  if (my_strcmp(word, "quit") == 0)
  {
    quit = execute_quit(command);
  }
  if (my_strcmp(word, "help") == 0)
  {
    quit = execute_help(command);
  }
  else if (my_strcmp(word, "paging") == 0)
  {
    quit = execute_paging(command);
  }
  else if (my_strcmp(word, "pf") == 0)
  {
    quit = execute_pf(command);
  }
  else if (my_strcmp(word, "test") == 0)
  {
    quit = execute_test(command);
  }
  else if (my_strcmp(word, "pwrstats") == 0)
  {
    quit = execute_rapl(command);
  }
  else if (my_strcmp(word, "threading") == 0)
  {
    quit = execute_threading(command);
  }
  else /* default: */
  {
    print("command not recognized");
  }
  // vga_attr = vga_make_color(COLOR_PROMPT_FOREGROUND, COLOR_PROMPT_BACKGROUND);
  return quit;

}

static int nk_monitor_loop()
{
  int buffer_size = 80 * 2;
  char buffer[buffer_size];

  // Inner loop is indvidual keystrokes, outer loop handles commands
  int done = 0;
  while (!done) {
    DS("monitor> ");
    wait_for_command(buffer, buffer_size);
    serial_putchar('\n');
    done = execute_potential_command(buffer);
  };
  return 0;
}



// ordinary reques for entry into the monitor
// we just wait our turn and then alert everyone else
static int monitor_init_lock()
{
    return 0; // handle processing...
}


// Main CPU wraps up its use of the monitor and updates everyone
static int monitor_deinit_lock(void)
{
    return 0;
}

// every entry to the monitor
// returns int flag for later restore
static uint8_t monitor_init(void)
{

    monitor_init_lock();

    // vga_copy_out(screen_saved, VGA_WIDTH*VGA_HEIGHT*2);
    // vga_get_cursor(&cursor_saved_x, &cursor_saved_y);

    // vga_x = vga_y = 0;
    // vga_attr = vga_make_color(COLOR_PROMPT_FOREGROUND, COLOR_PROMPT_BACKGROUND);
    // vga_clear_screen(vga_make_entry(' ', vga_attr));
    // vga_set_cursor(vga_x, vga_y);

    // kbd_flags = 0;

    // probably should reset ps2 here...

    return 0;

}

// called right before every exit from the monitor
// takes int flag to restore
static void monitor_deinit(uint8_t intr_flags)
{
    // vga_copy_in(screen_saved, VGA_WIDTH*VGA_HEIGHT*2);
    // vga_set_cursor(cursor_saved_x, cursor_saved_y);

    monitor_deinit_lock();

}

// Entrypoints to the monitor

// Entering through the shell command or f9
int my_monitor_entry()
{
    uint8_t intr_flags = monitor_init();

    // vga_attr = vga_make_color(COLOR_FOREGROUND, COLOR_BACKGROUND);
    print("+++ Boot into Monitor +++");
    // vga_attr = vga_make_color(COLOR_PROMPT_FOREGROUND, COLOR_PROMPT_BACKGROUND);

    nk_monitor_loop();

    monitor_deinit(intr_flags);

    return 0;
}
