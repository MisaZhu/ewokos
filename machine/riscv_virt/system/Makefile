EWOKOS_ROOT_DIR = ../../..
SYS_ROOT_DIR = ${EWOKOS_ROOT_DIR}/system
INST_DIR = /usr/local/ewokos
BUILD_DIR = $(SYS_ROOT_DIR)/build
MACHINE_BUILD_DIR = build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=virt

export ARCH=riscv
export NEON=yes
export BSP_LFLAGS= -larch_virt

all: x_sys
	@echo "all done."
	rm -fr $(MACHINE_BUILD_DIR)
	mkdir -p $(MACHINE_BUILD_DIR)
	mv $(TARGET_DIR)/* $(MACHINE_BUILD_DIR)
	rm -fr $(TARGET_DIR)

include $(SYS_ROOT_DIR)/platform/riscv/make.inc

x_sys: kernel_heads 
	rm -f $(BUILD_DIR)/lib/libbsp.a

	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	cd libs;make
	cd $(SYS_ROOT_DIR)/basic;make
	cd $(SYS_ROOT_DIR)/gui;make
	cd drivers;make
	cd $(SYS_ROOT_DIR)/xwin;make
	cp -r etc $(TARGET_DIR)/

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(SYS_ROOT_DIR)/xwin;make clean
	cd $(SYS_ROOT_DIR)/gui;make clean
	cd $(SYS_ROOT_DIR)/basic;make clean
	rm -fr $(SYS_ROOT_DIR)/build/$(HW)
	rm -fr $(MACHINE_BUILD_DIR)

install:
	mkdir -p $(INST_DIR)/$(ARCH)/$(HW)
	cp -r $(BUILD_DIR)/include $(INST_DIR)/$(ARCH)/$(HW)/
	cp -r $(BUILD_DIR)/lib $(INST_DIR)/$(ARCH)/$(HW)/ 
	cp $(SYS_ROOT_DIR)/platform/$(ARCH)/make.rule $(INST_DIR)/$(ARCH)/