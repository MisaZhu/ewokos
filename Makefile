OS = EwokOS

ifndef arch
	arch = versatilepb
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
CFLAGS = -mcpu=$(CPU) -gstabs -I include \
				 -marm \
				 -std=c99 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit
ASFLAGS = -mcpu=$(CPU) -g -I include
QEMU_FLAGS = $(ARCH_QEMU_FLAGS) -nographic

all: $(OS).bin

LIB_OBJS = lib/string.o

OBJS = kernel/asm/boot.o kernel/asm/system.o kernel/asm/context.o \
	$(LIB_OBJS) \
	kernel/startup.o \
	kernel/hardware.o \
	kernel/kernel.o \
	kernel/irq.o \
	kernel/syscall.o \
	kernel/kalloc.o \
	kernel/mmu.o \
	kernel/proc.o 


$(OS).bin: $(OBJS) $(OS).ld 
	mkdir -p build
	$(LD) -T $(OS).ld $(OBJS) -o build/$(OS).elf
	$(OBJCOPY) -O binary build/$(OS).elf build/$(OS).bin
	$(OBJDUMP) -D build/$(OS).elf > build/$(OS).asm

qemu: $(OS).bin
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).bin

debug: $(OS).bin
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel build/$(OS).bin

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb build/$(OS).elf -x /tmp/gdbinit

clean:
	rm -f $(OBJS) $(EXTRA_CLEAN)
	rm -fr build
