include ../make.rule

export QEMU_AUDIO_DRV=none

BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

KERNEL_DIR=../../kernel/basic/kernel

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

LIB_INC = -I$(BUILD_DIR)/include
CFLAGS += $(LIB_INC)

all: mkdirs basic_libs builtin_apps basic_drivers
	@echo "all done."

KERNEL_H = \
	$(KERNEL_DIR)/include/proto_t.h \
	$(KERNEL_DIR)/include/syscalls.h \
	$(KERNEL_DIR)/include/signals.h \
	$(KERNEL_DIR)/include/interrupt.h \
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
	cd $(ROOT_DIR)/libs/screen;make
	cd $(ROOT_DIR)/libs/graph;make
	cd $(ROOT_DIR)/libs/fb;make
	cd $(ROOT_DIR)/libs/fonts;make
	cd $(ROOT_DIR)/libs/upng; make
	cd $(ROOT_DIR)/libs/console;make

basic_drivers:
	cd $(ROOT_DIR)/drivers/screend;make
	cd $(ROOT_DIR)/drivers/consoled;make

include $(ROOT_DIR)/sbin/init/build.mk
include $(ROOT_DIR)/sbin/core/build.mk
include $(ROOT_DIR)/sbin/vfsd/build.mk
include $(ROOT_DIR)/sbin/rootfs/sdfs/build.mk

include $(ROOT_DIR)/drivers/nulld/build.mk
include $(ROOT_DIR)/drivers/dupd/build.mk
include $(ROOT_DIR)/drivers/ramfsd/build.mk
include $(ROOT_DIR)/drivers/proc/sysinfo/build.mk
include $(ROOT_DIR)/drivers/proc/state/build.mk

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/bin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/dev
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/proc
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/drivers/proc
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/home/root
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/home/guest
	@mkdir -p ${BUILD_DIR}/asm
	@mkdir -p ${BUILD_DIR}/include
	@mkdir -p ${BUILD_DIR}/lib
	@cp -r $(KERNEL_H) $(BUILD_DIR)/include/
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)
	@cp -r ${ROOT_DIR}/data ${TARGET_DIR}/$(ROOT_DIR)

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
	cd $(ROOT_DIR)/bin/grep;make

clean:
	rm -f $(CLEAN)
	cd $(ROOT_DIR)/libs/libc;make clean
	cd $(ROOT_DIR)/libs/hash;make clean
	cd $(ROOT_DIR)/libs/ext2;make clean
	cd $(ROOT_DIR)/libs/arch_vpb;make clean
	cd $(ROOT_DIR)/libs/arch_bcm283x;make clean
	cd $(ROOT_DIR)/libs/sd;make clean
	cd $(ROOT_DIR)/libs/fb;make clean
	cd $(ROOT_DIR)/libs/graph;make clean
	cd $(ROOT_DIR)/libs/fonts;make clean
	cd $(ROOT_DIR)/libs/console;make clean
	cd $(ROOT_DIR)/libs/screen;make clean
	cd $(ROOT_DIR)/libs/upng; make clean
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
	cd $(ROOT_DIR)/bin/grep;make clean
	cd $(ROOT_DIR)/drivers/screend;make clean
	cd $(ROOT_DIR)/drivers/consoled;make clean
