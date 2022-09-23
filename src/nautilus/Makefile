obj-y += \
	fmtout.o \
	printk.o \
	doprnt.o \
	vc.o \
	idt.o \
	queue.o \
	irq.o \
	naut_string.o \
	msr.o \
	cpuid.o \
	crypto.o \
	mtrr.o \
	fpu.o \
	spinlock.o \
	rwlock.o \
	condvar.o \
	semaphore.o \
	msg_queue.o \
	hashtable.o \
	rbtree.o \
	random.o \
	smp.o \
	idle.o \
	thread.o \
	task.o \
	future.o \
	waitqueue.o \
	group.o \
	timer.o \
	scheduler.o \
	group_sched.o \
	barrier.o \
	backtrace.o \
	cpu.o \
	acpi.o \
	numa.o \
	env.o \
	nemo.o \
	pmc.o \
	setjmp.o \
	mm/ \
	dev.o \
	chardev.o \
	blkdev.o \
	netdev.o \
	gpudev.o \
	fs.o \
	shell.o \
	fprintk.o \
	dtoa.o \
	module.o \
	linker.o \
	prog.o \
	getopt.o \
	cmdline.o \
	lru_cache.o

obj-$(NAUT_CONFIG_ARCH_X86) += \
	mb_utils.o \
	paging.o \
	msr.o \
	cpuid.o \
	mtrr.o \
	ticketlock.o \
	cpu.o \
	acpi.o \
	numa.o \
	nemo.o \
	pmc.o \
	setjmp.o \
	loader.o \

obj-$(NAUT_CONFIG_ARCH_RISCV) += devicetree.o

obj-$(NAUT_CONFIG_PROFILE) += instrument.o
obj-$(NAUT_CONFIG_XEON_PHI) += sfi.o

obj-$(NAUT_CONFIG_PALACIOS) += vmm.o

obj-$(NAUT_CONFIG_REAL_MODE_INTERFACE) += realmode.o

obj-$(NAUT_CONFIG_ISOCORE) += isocore.o

obj-$(NAUT_CONFIG_ENABLE_REMOTE_DEBUGGING) += gdb-stub.o

obj-$(NAUT_CONFIG_ENABLE_MONITOR) += monitor.o

obj-$(NAUT_CONFIG_WATCHDOG) +=  watchdog.o

obj-$(NAUT_CONFIG_CACHEPART) +=	cachepart.o

obj-$(NAUT_CONFIG_FIBER_ENABLE) += fiber.o

obj-$(NAUT_CONFIG_ASPACES) +=  aspace.o 

obj-$(NAUT_CONFIG_ALLOCS) +=  alloc.o 

obj-$(NAUT_CONFIG_PARTITION_SUPPORT) += partition.o

obj-$(NAUT_CONFIG_PROVENANCE) += provenance.o

obj-$(NAUT_CONFIG_PROCESSES) += process.o \
								signal.o

obj-$(NAUT_CONFIG_LINUX_SYSCALLS) += syscalls/

obj-$(NAUT_CONFIG_USE_IST) += gdt.o
