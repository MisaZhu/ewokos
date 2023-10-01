ROOT_DIR = ../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs


all: basic_sys
	@echo "all done."

include ../make.inc

extra: extra_sys
	@echo "all done."

full: full_sys
	@echo "all done."

basic_sys: kernel_heads 
	mkdir -p $(TARGET_DIR)/drivers/versatilepb
	cd libs;make
	cd $(ROOT_DIR)/basic;make
	cd drivers;make basic_sys
	@cp -r etc/basic/* ${TARGET_DIR}/etc

full_sys: basic_sys 
	cd $(ROOT_DIR)/full;make
	cd drivers;make full_sys
	@cp -r etc/full/* ${TARGET_DIR}/etc

extra_sys: full_sys
	cd $(ROOT_DIR)/extra;make

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(ROOT_DIR)/full;make clean
	cd $(ROOT_DIR)/basic;make clean
	cd $(ROOT_DIR)/extra;make clean
	rm -fr $(ROOT_DIR)/build

run:
	qemu-system-arm -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -kernel $(ROOT_DIR)/../kernel/hardware/arm/versatilepb/kernel.qemu.img -sd build/root.ext2
