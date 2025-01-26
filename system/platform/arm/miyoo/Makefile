ROOT_DIR = ../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=miyoo

export ARCH=arm
export G_2D_BOOST=yes
export BSP_LFLAGS= -larch_miyoo

all: basic_sys
	@echo "all done."
	rm -fr $(BUILD_DIR)/$(HW)
	mkdir -p $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR)/* $(BUILD_DIR)/$(HW)
	cp -r $(BUILD_DIR)/include $(BUILD_DIR)/$(HW)
	cp -r $(BUILD_DIR)/lib $(BUILD_DIR)/$(HW)
	rm -fr $(TARGET_DIR)
	rm -fr $(BUILD_DIR)/include
	rm -fr $(BUILD_DIR)/lib

include ../make.inc

basic_sys: kernel_heads 
	rm -f $(BUILD_DIR)/lib/libbsp.a

	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	cd libs;make
	cd $(ROOT_DIR)/basic;make
	cd $(ROOT_DIR)/full;make
	cd drivers;make
	cp -r etc $(TARGET_DIR)/

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(ROOT_DIR)/full;make clean
	cd $(ROOT_DIR)/basic;make clean
	rm -fr $(ROOT_DIR)/build/$(HW)
