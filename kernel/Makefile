OS = mkos

# tools
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

export QEMU_AUDIO_DRV=none

ifneq ($(MACH),)
ARCH=arm/$(MACH)
else
ARCH=arm/versatilepb
endif

ARCH_DIR=arch
include ${ARCH_DIR}/${ARCH}/config.mk

# flags
#OPTM = -g
OPTM = -O2

CFLAGS += $(OPTM) \
	-I. -I../include -Iinclude \
	-I${ARCH_DIR}/${ARCH} \
	-marm $(ARCH_QEMU_CFLAGS) \
	-Wstrict-prototypes \
	-pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
	-fno-builtin-printf \
	-fno-builtin-strcpy \
	-Wno-overlength-strings \
	-fno-builtin-exit \
	-fno-builtin-stdio \
	-fno-builtin-memset \
	-fno-builtin-memcpy \
	-fno-builtin-strchr \
	-fno-builtin-strcmp \
	-fno-builtin-strlen \
	-fno-builtin-strncpy \
	-fno-builtin-strncmp \
	-std=c99

LDS = mkos.lds.sd.S

ASFLAGS = $(OPTM) -I. -Iinclude -marm $(ARCH_QEMU_CFLAGS)

all: $(OS).bin 

ARCH_OBJS = \
	${ARCH_DIR}/${ARCH}/boot.o \
	${ARCH_DIR}/arm/common/interrupt.o \
	${ARCH_DIR}/arm/common/system.o \
	${ARCH_DIR}/arm/common/memcpy.o \
	${ARCH_DIR}/${ARCH}/hw_info.o \
	${ARCH_DIR}/${ARCH}/uart.o \
	${ARCH_DIR}/${ARCH}/irq_arch.o \
	${ARCH_DIR}/${ARCH}/framebuffer.o \
	${ARCH_DIR}/${ARCH}/actled.o \
	${ARCH_DIR}/${ARCH}/sd.o \
	${ARCH_DIR}/${ARCH}/timer.o \
	${ARCH_DIR}/${ARCH}/mailbox.o 

ifeq ($(MACH),raspi)
CFLAGS += -I${ARCH_DIR}/${ARCH}/extra -DWITH_LCDHAT

ARCH_OBJS += ${ARCH_DIR}/${ARCH}/extra/lib/gpio_arch.o  \
	${ARCH_DIR}/${ARCH}/extra/lib/spi_arch.o \
	${ARCH_DIR}/${ARCH}/extra/lcdhat/lcdhat.o \
	${ARCH_DIR}/${ARCH}/cpu_freq.o
endif

LIB_DIR = lib
LIB_OBJS = \
	${LIB_DIR}/graph.o \
	${LIB_DIR}/font.o \
	${LIB_DIR}/font8x16.o \
	${LIB_DIR}/console.o \
	${LIB_DIR}/kstring.o \
	${LIB_DIR}/buffer.o \
	${LIB_DIR}/vprintf.o \
	${LIB_DIR}/proto.o \
	${LIB_DIR}/mstr.o \
	${LIB_DIR}/charbuf.o \
	${LIB_DIR}/basic_math.o \
	${LIB_DIR}/ext2read.o \
	${LIB_DIR}/kprintf.o

KERNEL_DIR = kernel
OBJS = ${ARCH_OBJS} \
	${LIB_OBJS} \
	${KERNEL_DIR}/mm/startup.o \
	${KERNEL_DIR}/mm/kalloc.o \
	${KERNEL_DIR}/mm/mmu.o \
	${KERNEL_DIR}/mm/trunkmalloc.o \
	${KERNEL_DIR}/mm/kmalloc.o \
	${KERNEL_DIR}/mm/shm.o \
	${KERNEL_DIR}/init.o \
	${KERNEL_DIR}/uspace_int.o \
	${KERNEL_DIR}/irq.o \
	${KERNEL_DIR}/env.o \
	${KERNEL_DIR}/svc.o \
	${KERNEL_DIR}/proc.o \
	${KERNEL_DIR}/ipc.o \
	${KERNEL_DIR}/schedule.o \
	${KERNEL_DIR}/vfs.o \
	${KERNEL_DIR}/dev/kdevice.o \
	${KERNEL_DIR}/systemc.o \
	${KERNEL_DIR}/kernel.o

BUILD_DIR = build
SYS_DIR = ../system

$(OS).bin: $(OBJS) ${LDS}
	mkdir -p ${BUILD_DIR}
	$(LD) -L ${ARCH_DIR}/${ARCH} -T ${LDS} $(OBJS) -o ${BUILD_DIR}/$(OS).elf
	$(OBJCOPY) -O binary ${BUILD_DIR}/$(OS).elf ${BUILD_DIR}/$(OS).bin
	$(OBJDUMP) -D ${BUILD_DIR}/$(OS).elf > ${BUILD_DIR}/$(OS).asm

SDEXT2 = $(SYS_DIR)/build/sd.ext2

run: 
	qemu-system-arm $(QEMU_FLAGS) -kernel ${BUILD_DIR}/$(OS).bin -sd ${SDEXT2}

runasm: 
	qemu-system-arm $(QEMU_FLAGS) -kernel ${BUILD_DIR}/$(OS).bin -sd ${SDEXT2} -d in_asm

debug:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel ${BUILD_DIR}/$(OS).bin -sd ${SDEXT2}

debugasm:
	qemu-system-arm $(QEMU_FLAGS) -d in_asm -gdb tcp::26000 -S -kernel ${BUILD_DIR}/$(OS).bin

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	#arm-none-eabi-gdb ${BUILD_DIR}/$(OS).elf -x /tmp/gdbinit
	gdb-multiarch ${BUILD_DIR}/$(OS).elf -x /tmp/gdbinit #for ubuntu 18.04 without arm-none-eabi-gdb

clean:
	rm -f $(OBJS)
	rm -fr ${BUILD_DIR}
