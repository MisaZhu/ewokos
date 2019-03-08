OS = EwokOS

ifndef arch
#	arch = versatilepb
	arch = raspi2
endif
include arch/$(arch)/config.mk

# tools
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

# flags
CFLAGS = -I. -Iinclude -I../lib/include \
				 -marm   $(ARCH_QEMU_CFLAGS) \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
				 -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
				 -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 \
				 -g \
				 -mcpu=$(CPU)

ASFLAGS = -g -I. -I include -I../lib/include -mcpu=$(CPU) $(ARCH_QEMU_CFLAGS)
QEMU_FLAGS = $(ARCH_QEMU_FLAGS)

all: $(OS).bin 

OBJS = asm/boot.o \
			 asm/system.o \
			 asm/context.o

ARCH_OBJS = arch/$(arch)/src/irq.o \
	arch/$(arch)/src/uart.o \
	arch/$(arch)/src/timer.o \
	arch/$(arch)/src/mailbox.o \
	arch/$(arch)/src/fb.o \
	arch/$(arch)/src/hardware.o

DEV_OBJS = src/dev/uart.o

LIB_OBJS = ../lib/src/kstring.o \
	../lib/src/base16.o \
	../lib/src/sramdisk.o \
	../lib/src/vsprintf.o \
	../lib/src/tree.o

OBJS += $(ARCH_OBJS) \
	$(DEV_OBJS) \
	src/system.o \
	src/printk.o \
	src/kernel.o \
	src/irq.o \
	src/syscalls.o \
	src/mm/startup.o \
	src/mm/mmu.o \
	src/mm/kalloc.o \
	src/mm/trunkmalloc.o \
	src/mm/kmalloc.o \
	src/mm/shm.o \
	src/proc.o \
	src/kipc.o \
	src/kfile.o \
	src/vfs.o \
	src/fstree.o \
	src/kserv.o \
	src/scheduler.o

$(OS).bin: $(OBJS) $(LIB_OBJS) $(OS).ld
	mkdir -p build
	$(LD) -L arch/$(arch) -T $(OS).ld $(OBJS) $(LIB_OBJS) -o build/$(OS).elf
	$(OBJCOPY) -O binary build/$(OS).elf build/$(OS).bin
	$(OBJDUMP) -D build/$(OS).elf > build/$(OS).asm

run_nogra:
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).elf -nographic

run:
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).elf -serial mon:stdio

runasm:
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).elf -d in_asm   -serial mon:stdio

debug: $(OS).bin
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel build/$(OS).elf -serial mon:stdio

debug_nogra: $(OS).bin
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel build/$(OS).elf -initrd $(INITRAMFS) -nographic

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb build/$(OS).elf -x /tmp/gdbinit

clean:
	rm -f $(OBJS) $(EXTRA_CLEAN) $(LIB_OBJS)
	rm -fr build
