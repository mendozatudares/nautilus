#include <nautilus/arch.h>

void arch_enable_ints(void)  { asm volatile ("sti" : : : "memory"); }
void arch_disable_ints(void) { asm volatile ("cli" : : : "memory"); }
int  arch_ints_enabled(void) {
    uint64_t rflags = 0;
    asm volatile("pushfq; popq %0" : "=a"(rflags));
    return (rflags & RFLAGS_IF) != 0;
}

#include <dev/apic.h>
#define MY_APIC per_cpu_get(system)->cpus[my_cpu_id()]->apic

uint32_t arch_cycles_to_ticks(uint64_t cycles) {
    return apic_cycles_to_ticks(MY_APIC, cycles);
}
uint32_t arch_realtime_to_ticks(uint64_t ns) {
    return apic_realtime_to_ticks(MY_APIC, ns);
}
uint64_t arch_realtime_to_cycles(uint64_t ns) {
    return apic_realtime_to_cycles(MY_APIC, ns);
}
uint64_t arch_cycles_to_realtime(uint64_t cycles) {
    return apic_cycles_to_realtime(MY_APIC, cycles);
}

#include <nautilus/scheduler.h>

void arch_update_timer(uint32_t ticks, nk_timer_condition_t cond) {
    apic_update_oneshot_timer(MY_APIC, ticks, cond);
}
void arch_set_timer(uint32_t ticks) { apic_set_oneshot_timer(MY_APIC, ticks); }
int  arch_read_timer(void) { return apic_read_timer(MY_APIC); }
uint64_t arch_read_timestamp() { return rdtsc(); }

int apic_timer_handler(excp_entry_t * excp, excp_vec_t vec, void *state);
int  arch_timer_handler(excp_entry_t * excp, excp_vec_t vec, void *state) {
    return apic_timer_handler(excp, vec, state);
}

void arch_print_regs(struct nk_regs * r) {

    int i = 0;
    ulong_t cr0 = 0ul;
    ulong_t cr2 = 0ul;
    ulong_t cr3 = 0ul;
    ulong_t cr4 = 0ul;
    ulong_t cr8 = 0ul;
    ulong_t fs  = 0ul;
    ulong_t gs  = 0ul;
    ulong_t sgs = 0ul;
    uint_t  fsi;
    uint_t  gsi;
    uint_t  cs;
    uint_t  ds;
    uint_t  es;
    ulong_t efer;

    printk("RIP: %04lx:%016lx\n", r->cs, r->rip);
    printk("RSP: %04lx:%016lx RFLAGS: %08lx Vector: %08lx Error: %08lx\n", 
            r->ss, r->rsp, r->rflags, r->vector, r->err_code);
    printk("RAX: %016lx RBX: %016lx RCX: %016lx\n", r->rax, r->rbx, r->rcx);
    printk("RDX: %016lx RDI: %016lx RSI: %016lx\n", r->rdx, r->rdi, r->rsi);
    printk("RBP: %016lx R08: %016lx R09: %016lx\n", r->rbp, r->r8, r->r9);
    printk("R10: %016lx R11: %016lx R12: %016lx\n", r->r10, r->r11, r->r12);
    printk("R13: %016lx R14: %016lx R15: %016lx\n", r->r13, r->r14, r->r15);

    asm volatile("movl %%cs, %0": "=r" (cs));
    asm volatile("movl %%ds, %0": "=r" (ds));
    asm volatile("movl %%es, %0": "=r" (es));
    asm volatile("movl %%fs, %0": "=r" (fsi));
    asm volatile("movl %%gs, %0": "=r" (gsi));

    gs  = msr_read(MSR_GS_BASE);
    fs  = msr_read(MSR_FS_BASE);
    gsi = msr_read(MSR_KERNEL_GS_BASE);
    efer = msr_read(IA32_MSR_EFER);

    asm volatile("movq %%cr0, %0": "=r" (cr0));
    asm volatile("movq %%cr2, %0": "=r" (cr2));
    asm volatile("movq %%cr3, %0": "=r" (cr3));
    asm volatile("movq %%cr4, %0": "=r" (cr4));
    asm volatile("movq %%cr8, %0": "=r" (cr8));

    printk("FS: %016lx(%04x) GS: %016lx(%04x) knlGS: %016lx\n", 
            fs, fsi, gs, gsi, sgs);
    printk("CS: %04x DS: %04x ES: %04x CR0: %016lx\n", 
            cs, ds, es, cr0);
    printk("CR2: %016lx CR3: %016lx CR4: %016lx\n", 
            cr2, cr3, cr4);
    printk("CR8: %016lx EFER: %016lx\n", cr8, efer);

}

void * arch_read_sp(void) {
    void * sp = NULL;
    __asm__ __volatile__ ( "movq %%rsp, %0" : "=r"(sp) : : "memory");
    return sp;
}

void arch_relax(void) {
    asm volatile ("pause");
}

void arch_halt(void) {
    asm volatile ("hlt");
}

