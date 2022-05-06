include ../make.rule

BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..
KERNEL_DIR=../../kernel/basic/kernel

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
	cd libs;make

basic_drivers:
	cd drivers;make


mkdirs:
	@mkdir -p ${TARGET_DIR}/sbin
	@mkdir -p ${TARGET_DIR}/etc
	@mkdir -p ${TARGET_DIR}/bin
	@mkdir -p ${TARGET_DIR}/dev
	@mkdir -p ${TARGET_DIR}/proc
	@mkdir -p ${TARGET_DIR}/drivers/proc
	@mkdir -p ${TARGET_DIR}/home/root
	@mkdir -p ${TARGET_DIR}/home/guest
	@mkdir -p ${BUILD_DIR}/asm
	@mkdir -p ${BUILD_DIR}/include
	@mkdir -p ${BUILD_DIR}/lib
	@cp -r $(KERNEL_H) $(BUILD_DIR)/include/
	@cp -r ${ROOT_DIR}/etcs/qemu/basic/* ${TARGET_DIR}/etc
	@cp -r data ${TARGET_DIR}

builtin_apps: 
	cd sbin;make
	cd bin;make

clean:
	cd sbin;make clean
	cd bin;make clean
	cd libs;make clean
	cd drivers;make clean
