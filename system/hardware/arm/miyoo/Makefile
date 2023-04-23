ROOT_DIR = ../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs


all: basic_sys
	@echo "all done."

include ../make.inc

extra: basic_sys extra_sys
	@echo "all done."

basic_sys: kernel_heads 
	mkdir -p $(TARGET_DIR)/drivers/miyoo
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
