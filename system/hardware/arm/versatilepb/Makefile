ROOT_DIR = ../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs


all: basic_sys
	@echo "all done."

include ../make.inc

extra: basic_sys extra_sys
	@echo "all done."

basic_sys: kernel_heads 
	mkdir -p $(TARGET_DIR)/drivers/versatilepb
	cd libs;make
	cd $(ROOT_DIR)/basic;make
	cd $(ROOT_DIR)/full;make
	cd drivers;make
	cp -r etc $(TARGET_DIR)/

extra_sys:
	cd $(ROOT_DIR)/extra;make

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(ROOT_DIR)/full;make clean
	rm -fr $(ROOT_DIR)/build

run:
	qemu-system-arm -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -kernel $(ROOT_DIR)/../kernel/hardware/arm/versatilepb/kernel.qemu.img -sd build/root.ext2
