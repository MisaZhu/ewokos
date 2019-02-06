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
CFLAGS = -gstabs -I. -I src -I lib/include \
				 -I lib/include \
				 -I arch/$(arch)/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 \
				 -mcpu=$(CPU) 

ASFLAGS = -g -I src -I lib/include -mcpu=$(CPU) $(ARCH_QEMU_CFLAGS)
QEMU_FLAGS = $(ARCH_QEMU_FLAGS) -nographic

all: lib/libewok.a $(OS).bin 

OBJS = asm/boot.o \
	asm/system.o \
	asm/context.o

include lib/build.mk

ARCH_OBJS = arch/$(arch)/src/irq.o \
	arch/$(arch)/src/uart.o \
	arch/$(arch)/src/timer.o \
	arch/$(arch)/src/hardware.o

DEV_OBJS = src/dev/uart.o

OBJS += $(ARCH_OBJS) \
	$(DEV_OBJS) \
	src/kernel.o \
	src/irq.o \
	src/syscalls.o \
	src/mm/startup.o \
	src/mm/mmu.o \
	src/mm/kalloc.o \
	src/mm/trunkmalloc.o \
	src/mm/kmalloc.o \
	src/proc.o \
	src/kmessage.o \
	src/kfile.o \
	src/kserv.o \
	src/scheduler.o 

$(OS).bin: $(OBJS) $(OS).ld
	mkdir -p build
	$(LD) -L arch/$(arch) -T $(OS).ld $(OBJS) lib/libewok.a -o build/$(OS).elf
	$(OBJCOPY) -O binary build/$(OS).elf build/$(OS).bin
	$(OBJDUMP) -D build/$(OS).elf > build/$(OS).asm

run:
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).bin -initrd ../system/build/initfs

debug: $(OS).bin
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel build/$(OS).bin -initrd ../system/build/initfs -S

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb build/$(OS).elf -x /tmp/gdbinit

clean:
	rm -f $(OBJS) $(EXTRA_CLEAN) lib/libewok.a
	rm -fr build
