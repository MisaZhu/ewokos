export HW=versatilepb
export ARCH=arm
export ARCH_VER=v6

EWOKOS_ROOT_DIR = ../../..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
INST_DIR = /usr/local/ewokos

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
MACHINE_BUILD_DIR = build
TARGET_DIR = $(BUILD_DIR)/rootfs

export BSP_LFLAGS = -lbsp -larch_vpb

all: x
	@echo "all done."

include $(SYS_ROOT_DIR)/platform/make.inc

final:
	rm -fr $(MACHINE_BUILD_DIR)
	mkdir -p $(MACHINE_BUILD_DIR)
	mv $(TARGET_DIR)/* $(MACHINE_BUILD_DIR)
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
	cd $(EWOKOS_ROOT_DIR)/sw.extra/; make HW=$(HW) clean
	rm -fr $(BUILD_DIR)
	rm -fr $(MACHINE_BUILD_DIR)

SD = root.ext2
sd: 
	@echo "\n====building ext2 format sdcard image===="
	mkdir -p build
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1k count=256k
#make ext2 fs with: block_size=1024 inode_size=128
	mke2fs -b 1024 -I 128 ${SD}
 
	cd $(MACHINE_BUILD_DIR); \
	find . -type f | e2cp -ap -G0 -O0 -d ../${SD}:/

	@echo "==== ext2 format sdcard image created  ====\n"
