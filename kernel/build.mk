KERNEL_DIR=kernel


ARCH_OBJS = $(KERNEL_DIR)/arch/$(arch)/src/irq.o \
	$(KERNEL_DIR)/arch/$(arch)/src/uart.o \
	$(KERNEL_DIR)/arch/$(arch)/src/timer.o

OBJS += $(KERNEL_DIR)/src/asm/boot.o $(KERNEL_DIR)/src/asm/system.o $(KERNEL_DIR)/src/asm/context.o \
	$(LIB_OBJS) \
	$(ARCH_OBJS) \
	$(KERNEL_DIR)/src/startup.o \
	$(KERNEL_DIR)/src/hardware.o \
	$(KERNEL_DIR)/src/$(KERNEL_DIR).o \
	$(KERNEL_DIR)/src/irq.o \
	$(KERNEL_DIR)/src/syscalls.o \
	$(KERNEL_DIR)/src/kalloc.o \
	$(KERNEL_DIR)/src/mmu.o \
	$(KERNEL_DIR)/src/proc.o \
	$(KERNEL_DIR)/src/scheduler.o \
	$(KERNEL_DIR)/src/console.o \
	$(KERNEL_DIR)/src/loadinit.o

