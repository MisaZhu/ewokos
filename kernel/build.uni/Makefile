OS = ewokos

ifndef arch
	arch = versatilepb
#	arch = raspi2
endif
include ../arch/arm/mach-$(arch)/config.mk

# tools
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

# flags
CFLAGS = \
	-I. -I../include -I../arch/arm/include \
	-marm $(QEMU_CFLAGS) \
	-Wstrict-prototypes \
	-pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
	-fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
	-fno-builtin-exit -fno-builtin-stdio \
	-std=c99 \
	-O2 \
	-mcpu=$(CPU)

ifeq ($(keymap2), yes)
CFLAGS += -DCONFIG_KEYMAP2
endif

ASFLAGS = -O2 -I. -I../include -mcpu=$(CPU) $(ARCH_QEMU_CFLAGS)

all: $(OS).bin 

OBJS = \
	../arch/arm/common/boot.o \
	../arch/arm/common/system.o \
	../arch/arm/common/context.o

ARCH_OBJS = \
	../arch/arm/mach-$(arch)/irq.o \
	../arch/arm/mach-$(arch)/timer.o \
	../arch/arm/mach-$(arch)/uart.o \
	../arch/arm/mach-$(arch)/keyboard.o \
	../arch/arm/mach-$(arch)/mouse.o \
	../arch/arm/mach-$(arch)/mailbox.o \
	../arch/arm/mach-$(arch)/fb.o \
	../arch/arm/mach-$(arch)/hardware.o \
	../arch/arm/mach-$(arch)/sdc.o

DEV_OBJS =  ../drivers/basic_dev.o \
	../drivers/kevent_dev.o

LIB_OBJS = \
	../lib/kstring.o \
	../lib/basic_math.o \
	../lib/device.o \
	../lib/sconf_parse.o \
	../lib/sconf.o \
	../lib/trunk.o \
	../lib/sdread.o \
	../lib/tstr.o \
	../arch/arm/common/memcpy.o \
	../lib/vprintf.o

OBJS += $(ARCH_OBJS) \
	$(DEV_OBJS) \
	../init/kernel.o \
	../kernel/system.o \
	../kernel/timer.o \
	../kernel/printk.o \
	../kernel/syscalls.o \
	../kernel/proc.o \
	../kernel/semaphore.o \
	../kernel/ipc.o \
	../kernel/kfile.o \
	../kernel/kserv.o \
	../kernel/scheduler.o \
	../mm/startup.o \
	../mm/mmu.o \
	../mm/kalloc.o \
	../mm/trunkmalloc.o \
	../mm/kmalloc.o \
	../mm/shm.o

$(OS).bin: $(OBJS) $(LIB_OBJS) ../arch/arm/common/ewokos.lds.S
	mkdir -p build
	$(LD) -L ../arch/arm/mach-$(arch) -T ../arch/arm/common/ewokos.lds.S $(OBJS) $(LIB_OBJS) -o build/$(OS).elf
	$(OBJCOPY) -O binary build/$(OS).elf build/$(OS).bin
	$(OBJDUMP) -D build/$(OS).elf > build/$(OS).asm

run:
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).bin -serial mon:stdio -sd ../../rootfs/rootfs.ext2

runasm:
	qemu-system-arm $(QEMU_FLAGS) -kernel build/$(OS).bin -d in_asm -serial mon:stdio -sd ../../rootfs/rootfs.ext2

debug:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel build/$(OS).bin -serial mon:stdio -sd ../../rootfs/rootfs.ext2

debugasm:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel build/$(OS).bin -serial mon:stdio -sd ../../rootfs/rootfs.ext2 -d in_asm


gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb build/$(OS).elf -x /tmp/gdbinit

clean:
	rm -f $(OBJS) $(EXTRA_CLEAN) $(LIB_OBJS)
	rm -fr build
