ARCH_OBJS = kernel/arch/$(arch)/src/irq.o \
	kernel/arch/$(arch)/src/uart.o \
	kernel/arch/$(arch)/src/timer.o

OBJS += kernel/src/asm/boot.o kernel/src/asm/system.o kernel/src/asm/context.o \
	$(LIB_OBJS) \
	$(ARCH_OBJS) \
	kernel/src/startup.o \
	kernel/src/hardware.o \
	kernel/src/kernel.o \
	kernel/src/irq.o \
	kernel/src/syscalls.o \
	kernel/src/kalloc.o \
	kernel/src/mmu.o \
	kernel/src/console.o \
	kernel/src/proc.o \
	kernel/src/scheduler.o \
	kernel/src/loadinit.o
