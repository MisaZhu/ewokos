ROOT_DIR = ../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=versatilepb

export ARCH=arm
#export G_2D_BOOST=yes
export BSP_LFLAGS = -larch_vpb

all: full
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

basic: basic_sys
	@echo "all done."
	rm -fr $(BUILD_DIR)/$(HW)
	mkdir -p $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR)/* $(BUILD_DIR)/$(HW)


full: full_sys
	@echo "all done."

basic_sys: kernel_heads 
	rm -f $(BUILD_DIR)/lib/libbsp.a

	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	cd libs;make
	cd $(ROOT_DIR)/basic;make
	cd drivers;make basic_sys
	@cp -r etc/basic/* ${TARGET_DIR}/etc

full_sys: basic_sys 
	cd $(ROOT_DIR)/full;make
	cd drivers;make full_sys
	@cp -r etc/full/* ${TARGET_DIR}/etc

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(ROOT_DIR)/full;make clean
	cd $(ROOT_DIR)/basic;make clean
	rm -fr $(ROOT_DIR)/build

run:
	qemu-system-arm -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -kernel $(ROOT_DIR)/../kernel/platform/arm/versatilepb/kernel.qemu.img -sd build/root.ext2

net:
	sudo qemu-system-arm -k en.US -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -nic vmnet-shared -kernel $(ROOT_DIR)/../kernel/platform/arm/versatilepb/kernel.qemu.img -sd build/root.ext2
