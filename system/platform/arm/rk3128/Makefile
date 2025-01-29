ROOT_DIR = ../../..
INST_DIR = /usr/local/ewokos
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=rk3128

export ARCH=arm
export G_2D_BOOST=yes
export BSP_LFLAGS= -larch_rk3128

all: basic_sys
	@echo "all done."
	rm -fr $(BUILD_DIR)/$(HW)
	mkdir -p $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR)/* $(BUILD_DIR)/$(HW)
	rm -fr $(TARGET_DIR)

include ../make.inc

basic_sys: kernel_heads 
	rm -f $(BUILD_DIR)/lib/libbsp.a

	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	cd libs;make
	cd $(ROOT_DIR)/basic;make
	cd $(ROOT_DIR)/gui;make
	cd drivers;make
	cd $(ROOT_DIR)/xwin;make
	cp -r etc $(TARGET_DIR)/

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(ROOT_DIR)/xwin;make clean
	cd $(ROOT_DIR)/gui;make clean
	cd $(ROOT_DIR)/basic;make clean
	rm -fr $(ROOT_DIR)/build/$(HW)

install:
	mkdir -p $(INST_DIR)/$(ARCH)/$(HW)
	cp -r $(BUILD_DIR)/include $(INST_DIR)/$(ARCH)/$(HW)/
	cp -r $(BUILD_DIR)/lib $(INST_DIR)/$(ARCH)/$(HW)/ 
	cp $(ROOT_DIR)/platform/$(ARCH)/make.rule $(INST_DIR)/$(ARCH)/