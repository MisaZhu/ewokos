# tools
AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

export QEMU_AUDIO_DRV=none

BUILD_DIR = build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

# flags
CFLAGS = -Wstrict-prototypes \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
         -fno-builtin-calloc \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
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

KERNEL_DIR=../../kernel
ARCH_DIR=$(KERNEL_DIR)/arch

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

LIB_DIR = ../full/libs

LIB_INC = -I $(LIB_DIR)/sys/include \
				  -I $(LIB_DIR)/libc/include \
				  -I $(LIB_DIR)/libm/include \
					-I $(LIB_DIR)/hash/include \
					-I $(KERNEL_DIR)/include
				 
LIB_SYS_DIR = $(LIB_DIR)/sys
LIB_LIBC_DIR = $(LIB_DIR)/libc
LIB_M_DIR = $(LIB_DIR)/libm
LIB_HASH_DIR = $(LIB_DIR)/hash

CFLAGS += $(LIB_INC)

LIB_HASH_OBJS = $(LIB_HASH_DIR)/src/hashmap.o

LIB_M_OBJS = $(LIB_M_DIR)/src/mstr.o \
	$(LIB_M_DIR)/src/mstrx.o \
	$(LIB_M_DIR)/src/marray.o \
	$(LIB_M_DIR)/src/mutf8.o

LIB_SYS_OBJS = $(LIB_SYS_DIR)/src/syscall.o \
	$(LIB_SYS_DIR)/src/cmain.o \
	$(LIB_SYS_DIR)/src/kserv.o \
	$(LIB_SYS_DIR)/src/charbuf.o \
	$(LIB_SYS_DIR)/src/vfs.o \
	$(LIB_SYS_DIR)/src/ipc.o \
	$(LIB_SYS_DIR)/src/vdevice.o \
	$(LIB_SYS_DIR)/src/proto.o \
	$(LIB_SYS_DIR)/src/thread.o \
	$(LIB_SYS_DIR)/src/basic_math.o \
	$(LIB_SYS_DIR)/src/shm.o \
	$(LIB_SYS_DIR)/src/gpio.o \
	$(LIB_SYS_DIR)/src/global.o \
	$(LIB_SYS_DIR)/src/proc.o \
	$(LIB_SYS_DIR)/src/lockc.o \
	$(LIB_SYS_DIR)/src/wait.o \
	$(LIB_SYS_DIR)/src/critical.o \
	$(LIB_SYS_DIR)/src/md5.o \
	$(LIB_SYS_DIR)/src/buffer.o \
	$(LIB_SYS_DIR)/src/mmio.o 

LIB_LIBC_OBJS = $(LIB_LIBC_DIR)/src/unistd.o \
	$(LIB_LIBC_DIR)/src/stdlib.o \
	$(LIB_LIBC_DIR)/src/fcntl.o \
	$(LIB_LIBC_DIR)/src/vprintf.o \
	$(LIB_LIBC_DIR)/src/pthread.o \
	$(LIB_LIBC_DIR)/src/dirent.o \
	$(LIB_LIBC_DIR)/src/stdio.o

LIB_OBJS = $(LIB_SYS_OBJS) \
	$(LIB_LIBC_OBJS) \
	$(LIB_HASH_OBJS) \
	$(LIB_M_OBJS)

CLEAN = $(LIB_OBJS)

SD = $(BUILD_DIR)/sd.ext2

all: mkdirs libs builtin_apps rootfsd init #extra_devs
	${BUILD_DIR}/genfile init  $(BUILD_DIR)/rootfs/sbin/init > $(KERNEL_DIR)/extra/loadinit/mem/init_data.c
	@echo "all done."

KERNEL_H = \
	$(KERNEL_DIR)/include/usinterrupt.h \
	$(KERNEL_DIR)/include/syscalls.h \
	$(KERNEL_DIR)/include/fbinfo.h

libs: $(LIB_OBJS)
	$(AR) rT $(TARGET_DIR)/lib/libewokc.a $(LIB_SYS_OBJS)  $(LIB_LIBC_OBJS) $(LIB_M_OBJS)
	@cp -r $(LIB_SYS_DIR)/include/* $(LIB_LIBC_DIR)/include/* $(LIB_M_DIR)/include/* $(TARGET_DIR)/include/
	$(AR) rT $(TARGET_DIR)/lib/libhash.a $(LIB_HASH_OBJS)
	@cp -r $(LIB_HASH_DIR)/include/* $(TARGET_DIR)/include/
	@cp -r $(KERNEL_H) $(TARGET_DIR)/include/

include $(ROOT_DIR)/sbin/vfsd/build.mk
include $(ROOT_DIR)/sbin/procd/build.mk
include $(ROOT_DIR)/sbin/dev/nulld/build.mk

include $(ROOT_DIR)/sbin/dev/arch/versatilepb/ttyd/build.mk

include $(ROOT_DIR)/sbin/dev/arch/raspi2/ttyd/build.mk

include $(ROOT_DIR)/sbin/dev/arch/raspi/ttyd/build.mk

include $(ROOT_DIR)/bin/session/build.mk
include $(ROOT_DIR)/bin/shell/build.mk
include $(ROOT_DIR)/bin/pwd/build.mk
include $(ROOT_DIR)/bin/cat/build.mk
include $(ROOT_DIR)/bin/ps/build.mk
include $(ROOT_DIR)/bin/echo/build.mk
include $(ROOT_DIR)/bin/ls/build.mk
include $(ROOT_DIR)/bin/mount/build.mk
include $(ROOT_DIR)/bin/uname/build.mk
include $(ROOT_DIR)/bin/grep/build.mk

include $(ROOT_DIR)/sbin/dev/rootfsd/build.mk
include $(ROOT_DIR)/sbin/init/build.mk

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/dev/versatilepb
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/dev/raspi
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/dev/raspi2
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/bin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/include
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/lib
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/dev
	@mkdir -p ${BUILD_DIR}/asm
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)


builtin_apps: ${PROGS}
	gcc -o ${BUILD_DIR}/genfile geninit/genfile.c
	gcc -o ${BUILD_DIR}/genfs   geninit/genfs.c
	${BUILD_DIR}/genfile vfsd  $(BUILD_DIR)/rootfs/sbin/vfsd > sbin/init/vfsd_data.c
	${BUILD_DIR}/genfs $(BUILD_DIR)/rootfs > sbin/dev/rootfsd/fs_data.c

init: ${INIT}

rootfsd: ${ROOTFSD}
	${BUILD_DIR}/genfile rootfsd  $(BUILD_DIR)/rootfs/sbin/dev/rootfsd > sbin/init/rootfsd_data.c

KERNEL=$(KERNEL_DIR)/build/mkos.bin
KERNEL_ELF=$(KERNEL_DIR)/build/mkos.elf
include ${ARCH_DIR}/${ARCH}/config.mk

run: 
	qemu-system-arm $(QEMU_FLAGS) -kernel ${KERNEL}

debug:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel ${KERNEL}

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	#arm-none-eabi-gdb ${KERNEL_ELF} -x /tmp/gdbinit
	gdb-multiarch ${KERNEL_ELF} -x /tmp/gdbinit #for ubuntu 18.04 without arm-none-eabi-gdb

clean:
	rm -f $(CLEAN)
	rm -fr build
