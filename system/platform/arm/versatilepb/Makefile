ROOT_DIR = ../../..
INST_DIR = /usr/local/ewokos
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=versatilepb

export ARCH=arm
#export NEON=yes
export BSP_LFLAGS = -larch_vpb

all: x
	@echo "all done."

include ../make.inc

final:
	rm -fr $(BUILD_DIR)/$(HW)
	mkdir -p $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR)/* $(BUILD_DIR)/$(HW)
	rm -fr $(TARGET_DIR)

basic: basic_sys final
	@echo "all done."

gui: gui_sys final
	@echo "all done."

basic_sys: kernel_heads 
	rm -f $(BUILD_DIR)/lib/libbsp.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	cd libs;make
	cd $(ROOT_DIR)/basic;make
	cd drivers;make basic
	@cp -r etc/basic/* ${TARGET_DIR}/etc

gui_sys: basic_sys 
	cd $(ROOT_DIR)/gui;make
	cd drivers;make gui
	@cp -r etc/gui/* ${TARGET_DIR}/etc

x: x_sys final
	@echo "all done."

x_sys: gui_sys 
	cd $(ROOT_DIR)/xwin;make
	@cp -r etc/xwin/* ${TARGET_DIR}/etc

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

run:
	qemu-system-arm -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -kernel $(ROOT_DIR)/../kernel/platform/arm/versatilepb/kernel.qemu.img -sd build/root.ext2

net:
	sudo qemu-system-arm -k en.US -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -nic vmnet-shared -kernel $(ROOT_DIR)/../kernel/platform/arm/versatilepb/kernel.qemu.img -sd build/root.ext2
