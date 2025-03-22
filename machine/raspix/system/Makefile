EWOKOS_ROOT_DIR = ../../..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
INST_DIR = /usr/local/ewokos
BUILD_DIR = $(SYS_ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
HW=raspix

export ARCH=arm
export NEON=yes
export BSP_LFLAGS= -larch_bcm283x

all: x
	@echo "all done."

include $(SYS_ROOT_DIR)/platform/arm/make.inc

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
	cd $(SYS_ROOT_DIR)/basic;make
	cd drivers;make basic
	@cp -r etc/basic/* ${TARGET_DIR}/etc

gui_sys: basic_sys 
	cd $(SYS_ROOT_DIR)/gui;make
	cd drivers;make gui
	@cp -r etc/gui/* ${TARGET_DIR}/etc

x: x_sys final
	@echo "all done."

x_sys: gui_sys 
	cd $(SYS_ROOT_DIR)/xwin;make
	@cp -r etc/xwin/* ${TARGET_DIR}/etc

clean:
	cd libs;make clean
	cd drivers;make clean
	cd $(SYS_ROOT_DIR)/xwin;make clean
	cd $(SYS_ROOT_DIR)/gui;make clean
	cd $(SYS_ROOT_DIR)/basic;make clean
	rm -fr $(SYS_ROOT_DIR)/build/$(HW)

SD = build/root.ext2
sd: 
	@echo "\n====building ext2 format sdcard image===="
	mkdir -p build
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1k count=256k
#make ext2 fs with: block_size=1024 inode_size=128
	mke2fs -b 1024 -I 128 ${SD}
 
	@cd $(SYS_ROOT_DIR)/build/$(HW); \
	find . -type f | e2cp -ap -G0 -O0 -d $(EWOKOS_ROOT_DIR)/machine/${HW}/system/${SD}:/

	@echo "==== ext2 format sdcard image created  ====\n"

install:
	mkdir -p $(INST_DIR)/$(ARCH)/$(HW)
	cp -r $(BUILD_DIR)/include $(INST_DIR)/$(ARCH)/$(HW)/
	cp -r $(BUILD_DIR)/lib $(INST_DIR)/$(ARCH)/$(HW)/ 
	cp $(SYS_ROOT_DIR)/platform/$(ARCH)/make.rule $(INST_DIR)/$(ARCH)/
