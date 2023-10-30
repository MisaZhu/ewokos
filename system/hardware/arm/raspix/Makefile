ROOT_DIR = ../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=raspix

export ARCH=arm
export G_2D_BOOST=yes

all: full
	@echo "all done."
	rm -fr $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR) $(BUILD_DIR)/$(HW)
	cp -r $(BUILD_DIR)/include $(BUILD_DIR)/$(HW)
	cp -r $(BUILD_DIR)/lib $(BUILD_DIR)/$(HW)

include ../make.inc

basic: basic_sys
	@echo "all done."

extra: extra_sys
	@echo "all done."
	rm -fr $(BUILD_DIR)/$(HW)
	mv $(TARGET_DIR) $(BUILD_DIR)/$(HW)
	cp -r $(BUILD_DIR)/include $(BUILD_DIR)/$(HW)
	cp -r $(BUILD_DIR)/lib $(BUILD_DIR)/$(HW)

full: full_sys
	@echo "all done."

basic_sys: kernel_heads 
	rm -f $(BUILD_DIR)/lib/libbsp.a

	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	cd libs;make
	cd $(ROOT_DIR)/basic;make
	cd drivers;make basic_sys
	@cp -r etcs/qemu/basic/* ${TARGET_DIR}/etc

full_sys: basic_sys 
	cd $(ROOT_DIR)/full;make
	cd drivers;make full_sys
	@cp -r etcs/qemu/full/* ${TARGET_DIR}/etc

extra_sys: full_sys
	cd $(ROOT_DIR)/extra;make

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(ROOT_DIR)/full;make clean
	cd $(ROOT_DIR)/basic;make clean
	cd $(ROOT_DIR)/extra;make clean
	rm -fr $(ROOT_DIR)/build/$(HW)

run:
	qemu-system-arm -M raspi2b -m 1024M -serial mon:stdio -kernel $(ROOT_DIR)/../kernel/hardware/arm/raspi/pix/kernel7.qemu.img -sd build/root.ext2
