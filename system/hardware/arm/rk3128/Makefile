ROOT_DIR = ../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=rk3128

export ARCH=arm
export 2D_BOOST=yes

all: basic_sys
	@echo "all done."
	rm -fr $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR) $(BUILD_DIR)/$(HW)
	mv $(BUILD_DIR)/include $(BUILD_DIR)/$(HW)
	mv $(BUILD_DIR)/lib $(BUILD_DIR)/$(HW)

include ../make.inc

extra: basic_sys extra_sys
	@echo "all done."
	rm -fr $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR) $(BUILD_DIR)/$(HW)
	mv $(BUILD_DIR)/include $(BUILD_DIR)/$(HW)
	mv $(BUILD_DIR)/lib $(BUILD_DIR)/$(HW)

basic_sys: kernel_heads 
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	rm -f $(BUILD_DIR)/lib/libbsp.a
	rm -f $(BUILD_DIR)/lib/libgraph.a
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
	cd $(ROOT_DIR)/basic;make clean
	cd $(ROOT_DIR)/extra;make clean
	rm -fr $(ROOT_DIR)/build
