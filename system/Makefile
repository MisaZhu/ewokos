# tools
AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

export QEMU_AUDIO_DRV=none

# flags
CFLAGS = -Wstrict-prototypes \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

#CFLAGS +=  -g
CFLAGS +=  -O2

ifneq ($(MACH),)
ARCH=arm/$(MACH)
else
ARCH=arm/versatilepb
endif

ARCH_DIR=../kernel/arch

LDFLAGS = -lc -lnosys

LIB_INC = -I libs/sys/include \
				  -I libs/sconf/include \
				  -I libs/libc/include \
				  -I libs/x/include \
				  -I libs/ext2/include \
				  -I libs/graph/include \
				  -I libs/libm/include \
				  -I libs/console/include \
				  -I libs/mario/include \
					-I ../kernel/include
				 
LIB_SYS_DIR = libs/sys/src
LIB_SCONF_DIR = libs/sconf/src
LIB_LIBC_DIR = libs/libc/src
LIB_X_DIR = libs/x/src
LIB_EXT2_DIR = libs/ext2/src
LIB_GRAPH_DIR = libs/graph/src
LIB_M_DIR = libs/libm/src
LIB_CONSOLE_DIR = libs/console/src
LIB_MARIO_DIR = libs/mario/src

CFLAGS += $(LIB_INC)

GRAPH_OBJS =	$(LIB_GRAPH_DIR)/graph.o \
	$(LIB_GRAPH_DIR)/tga.o \
	$(LIB_GRAPH_DIR)/font.o \
	$(LIB_GRAPH_DIR)/font4x6.o \
	$(LIB_GRAPH_DIR)/font5x12.o \
	$(LIB_GRAPH_DIR)/font6x8.o \
	$(LIB_GRAPH_DIR)/font7x9.o \
	$(LIB_GRAPH_DIR)/font8x8.o \
	$(LIB_GRAPH_DIR)/font9x8.o \
	$(LIB_GRAPH_DIR)/font8x10.o \
	$(LIB_GRAPH_DIR)/font8x16.o \
	$(LIB_GRAPH_DIR)/font9x16.o \
  $(LIB_GRAPH_DIR)/font12x16.o \
	$(LIB_GRAPH_DIR)/font10x20.o \
	$(LIB_GRAPH_DIR)/font12x24.o \
	$(LIB_GRAPH_DIR)/font16x32.o

LIB_X_OBJS = $(LIB_X_DIR)/xclient.o

LIB_EXT2_OBJS = $(LIB_EXT2_DIR)/ext2fs.o

LIB_CONSOLE_OBJS = $(LIB_CONSOLE_DIR)/console.o

LIB_SCONF_OBJS = $(LIB_SCONF_DIR)/sconf.o

LIB_M_OBJS = $(LIB_M_DIR)/mstr.o \
	$(LIB_M_DIR)/mstrx.o \
	$(LIB_M_DIR)/marray.o \
	$(LIB_M_DIR)/mutf8.o

LIB_MARIO_OBJS = $(LIB_MARIO_DIR)/mario_bc.o \
	$(LIB_MARIO_DIR)/mario_json.o \
	$(LIB_MARIO_DIR)/mario_lex.o \
	$(LIB_MARIO_DIR)/mario_vm.o \
	$(LIB_MARIO_DIR)/compiler.o


LIB_SYS_OBJS = $(LIB_SYS_DIR)/syscall.o \
	$(LIB_SYS_DIR)/cmain.o \
	$(LIB_SYS_DIR)/vfs.o \
	$(LIB_SYS_DIR)/ipc.o \
	$(LIB_SYS_DIR)/vdevice.o \
	$(LIB_SYS_DIR)/proto.o \
	$(LIB_SYS_DIR)/thread.o \
	$(LIB_SYS_DIR)/basic_math.o \
	$(LIB_SYS_DIR)/shm.o \
	$(LIB_SYS_DIR)/sd.o \
	$(LIB_SYS_DIR)/gpio.o \
	$(LIB_SYS_DIR)/global.o \
	$(LIB_SYS_DIR)/proclock.o \
	$(LIB_SYS_DIR)/wait.o \
	$(LIB_SYS_DIR)/critical.o \
	$(LIB_SYS_DIR)/interrupt.o \
	$(LIB_SYS_DIR)/mmio.o 

LIB_LIBC_OBJS = $(LIB_LIBC_DIR)/unistd.o \
	$(LIB_LIBC_DIR)/stdlib.o \
	$(LIB_LIBC_DIR)/fcntl.o \
	$(LIB_LIBC_DIR)/vprintf.o \
	$(LIB_LIBC_DIR)/stdio.o

LIB_OBJS = $(LIB_SYS_OBJS) \
	$(LIB_SCONF_OBJS) \
	$(LIB_LIBC_OBJS) \
	$(GRAPH_OBJS) \
	$(LIB_EXT2_OBJS) \
	$(LIB_M_OBJS) \
	$(LIB_X_OBJS)


CLEAN = $(LIB_OBJS)

BUILD_DIR = build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .
SD = $(BUILD_DIR)/sd.ext2

all: mkdirs $(LIB_OBJS) apps
	@echo "all done."

include $(ROOT_DIR)/sbin/init/build.mk
include $(ROOT_DIR)/sbin/dev/fbd/build.mk
include $(ROOT_DIR)/sbin/dev/nulld/build.mk
include $(ROOT_DIR)/sbin/dev/xserverd/build.mk
include $(ROOT_DIR)/sbin/dev/rootfsd/build.mk
include $(ROOT_DIR)/sbin/x/xwm/build.mk

include $(ROOT_DIR)/sbin/dev/arch/versatilepb/keybd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/versatilepb/moused/build.mk
include $(ROOT_DIR)/sbin/dev/arch/versatilepb/ttyd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/versatilepb/sdd/build.mk

include $(ROOT_DIR)/sbin/dev/arch/raspi2/ttyd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi2/sdd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi2/actledd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi2/gpiod/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi2/spid/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi2/joystickd/build.mk

include $(ROOT_DIR)/sbin/dev/arch/raspi/ttyd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi/sdd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi/actledd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi/gpiod/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi/spid/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi/lcdhatd/build.mk
include $(ROOT_DIR)/sbin/dev/arch/raspi/joystickd/build.mk

include $(ROOT_DIR)/bin/shell/build.mk
include $(ROOT_DIR)/bin/pwd/build.mk
include $(ROOT_DIR)/bin/cat/build.mk
include $(ROOT_DIR)/bin/ps/build.mk
include $(ROOT_DIR)/bin/echo/build.mk
include $(ROOT_DIR)/bin/ls/build.mk
include $(ROOT_DIR)/bin/dump/build.mk
include $(ROOT_DIR)/bin/mount/build.mk
include $(ROOT_DIR)/bin/test/build.mk
include $(ROOT_DIR)/bin/uname/build.mk
include $(ROOT_DIR)/bin/tga/build.mk
include $(ROOT_DIR)/bin/gtest/build.mk
include $(ROOT_DIR)/bin/snake/build.mk
include $(ROOT_DIR)/bin/grep/build.mk
include $(ROOT_DIR)/bin/kill/build.mk
include $(ROOT_DIR)/bin/mkdir/build.mk
include $(ROOT_DIR)/bin/rm/build.mk
include $(ROOT_DIR)/bin/console/build.mk
include $(ROOT_DIR)/bin/xconsole/build.mk
include $(ROOT_DIR)/bin/launcher/build.mk
include $(ROOT_DIR)/bin/session/build.mk
include $(ROOT_DIR)/bin/x/build.mk
include $(ROOT_DIR)/bin/mario/build.mk

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/dev/versatilepb
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/dev/raspi
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/dev/raspi2
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/x
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/bin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/dev
	@mkdir -p ${BUILD_DIR}/asm
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)
	@cp -r ${ROOT_DIR}/data ${TARGET_DIR}/$(ROOT_DIR)

sd:
	@echo "\n====building ext2 format sdcard image===="
	dd if=/dev/zero of=${SD} bs=1024 count=16384
	mke2fs ${SD}
	mkdir -p tmp
	sudo fuse-ext2 -o force,rw+ ${SD} ./tmp
	sudo cp -r build/rootfs/* ./tmp
	sudo umount ./tmp
	rm -r tmp
	@echo "==== ext2 format sdcard image created  ====\n"

apps: ${PROGS}

KERNEL=../kernel/build/mkos.bin
KERNEL_ELF=../kernel/build/mkos.elf
include ${ARCH_DIR}/${ARCH}/config.mk

run: 
	qemu-system-arm $(QEMU_FLAGS) -kernel ${KERNEL} -sd ${SD}

debug:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel ${KERNEL} -sd ${SD}

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	#arm-none-eabi-gdb ${KERNEL_ELF} -x /tmp/gdbinit
	gdb-multiarch ${KERNEL_ELF} -x /tmp/gdbinit #for ubuntu 18.04 without arm-none-eabi-gdb

clean:
	rm -f $(CLEAN)
	rm -fr build
