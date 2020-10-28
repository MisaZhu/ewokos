# tools
AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

export QEMU_AUDIO_DRV=none

BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

# flags
CFLAGS = -Wstrict-prototypes \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-stat \
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

KERNEL_DIR=../../kernel/basic/kernel

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

LIB_INC = -I$(BUILD_DIR)/include
CFLAGS += $(LIB_INC)

all: mkdirs basic_libs builtin_apps
	@echo "all done."

KERNEL_H = \
	$(KERNEL_DIR)/include/syscalls.h \
	$(KERNEL_DIR)/include/signals.h \
	$(KERNEL_DIR)/include/sysinfo.h \
	$(KERNEL_DIR)/include/kevent.h \
	$(KERNEL_DIR)/include/procinfo.h

basic_libs:
	cd $(ROOT_DIR)/libs/libc;make
	cd $(ROOT_DIR)/libs/hash;make
	cd $(ROOT_DIR)/libs/ext2;make
	cd $(ROOT_DIR)/libs/arch_vpb;make
	cd $(ROOT_DIR)/libs/arch_bcm283x;make
	cd $(ROOT_DIR)/libs/sd;make

include $(ROOT_DIR)/sbin/init/build.mk

include $(ROOT_DIR)/drivers/nulld/build.mk
include $(ROOT_DIR)/drivers/klogd/build.mk
include $(ROOT_DIR)/drivers/stdiod/build.mk
include $(ROOT_DIR)/drivers/ramfsd/build.mk

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/bin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/dev
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/drivers
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/home/root
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/home/guest
	@mkdir -p ${BUILD_DIR}/asm
	@mkdir -p ${BUILD_DIR}/include
	@mkdir -p ${BUILD_DIR}/lib
	@cp -r $(KERNEL_H) $(BUILD_DIR)/include/
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)

builtin_apps: $(PROGS)
	cd $(ROOT_DIR)/bin/echo;make
	cd $(ROOT_DIR)/bin/sleep;make
	cd $(ROOT_DIR)/bin/ps;make
	cd $(ROOT_DIR)/bin/cat;make
	cd $(ROOT_DIR)/bin/shell;make
	cd $(ROOT_DIR)/bin/ls;make
	cd $(ROOT_DIR)/bin/login;make
	cd $(ROOT_DIR)/bin/session;make
	cd $(ROOT_DIR)/bin/mount;make
	cd $(ROOT_DIR)/bin/uname;make
	cd $(ROOT_DIR)/bin/pwd;make
	cd $(ROOT_DIR)/bin/dump;make
	cd $(ROOT_DIR)/bin/kill;make
	cd $(ROOT_DIR)/bin/rm;make
	cd $(ROOT_DIR)/bin/mkdir;make

clean:
	rm -f $(CLEAN)
	cd $(ROOT_DIR)/bin/echo;make clean
	cd $(ROOT_DIR)/bin/sleep;make clean
	cd $(ROOT_DIR)/bin/ps;make clean
	cd $(ROOT_DIR)/bin/cat;make clean
	cd $(ROOT_DIR)/bin/shell;make clean
	cd $(ROOT_DIR)/bin/ls;make clean
	cd $(ROOT_DIR)/bin/login;make clean
	cd $(ROOT_DIR)/bin/session;make clean
	cd $(ROOT_DIR)/bin/mount;make clean
	cd $(ROOT_DIR)/bin/uname;make clean
	cd $(ROOT_DIR)/bin/pwd;make clean
	cd $(ROOT_DIR)/bin/dump;make clean
	cd $(ROOT_DIR)/bin/kill;make clean
	cd $(ROOT_DIR)/bin/rm;make clean
	cd $(ROOT_DIR)/bin/mkdir;make clean
