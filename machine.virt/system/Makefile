export HW=virt

ifeq ($(ARCH),)
#export ARCH=arm
export ARCH=aarch64
endif

ifeq ($(ARCH),arm)
export ARCH_VER=v7
endif

ifeq ($(FS),)
FS=ext3
endif


EWOKOS_ROOT_DIR = ../..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
INST_DIR = /usr/local/ewokos

BUILD_DIR = $(SYS_ROOT_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
EXTRA_APPS_DIR = $(EWOKOS_ROOT_DIR)/extra.apps

export BSP_LFLAGS = -lbsp -larch_virt

all: x
	@echo "all done."

include $(SYS_ROOT_DIR)/platform/make.inc

basic: basic_sys
	@$(MAKE) final

network: network_sys
	@$(MAKE) final

gui: gui_sys
	@$(MAKE) final

x: x_sys
	@$(MAKE) final

basic_sys: kernel_heads
	rm -f $(BUILD_DIR)/lib/libbsp.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	cd libs;make
	cd $(SYS_ROOT_DIR)/basic;make
	cd drivers;make basic
	@cp -r etc/basic/* ${TARGET_DIR}/etc

network_sys: basic_sys
	cd $(SYS_ROOT_DIR)/network;make
	cd drivers;make network
	@cp -r etc/network/* ${TARGET_DIR}/etc

gui_sys: network_sys
	cd $(SYS_ROOT_DIR)/gui;make
	cd drivers;make gui
	@cp -r etc/gui/* ${TARGET_DIR}/etc

x_sys: gui_sys
	cd $(SYS_ROOT_DIR)/xwin;make
	@cp -r etc/xwin/* ${TARGET_DIR}/etc

CLEAN_DIRS = libs drivers $(SYS_ROOT_DIR)/xwin $(SYS_ROOT_DIR)/gui $(SYS_ROOT_DIR)/network $(SYS_ROOT_DIR)/basic
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	@$(MAKE) -C $(EWOKOS_ROOT_DIR)/sw.extra HW=$(HW) clean
	rm -fr $(BUILD_DIR)
	rm -fr $(FINAL_DIR)

SD = root_$(ARCH).img
sd:
	@echo "\n====building ${FS} format sdcard image===="
#512M ext2/ext3 sd
	dd if=/dev/zero of=${SD} bs=1k count=1024k
#make ext2/ext3 fs with: block_size=4096 inode_size=128
	mke2fs -t ${FS} -b 4096 -I 128 ${SD}

	cd $(FINAL_DIR); \
	find . -type f | e2cp -ap -G0 -O0 -d ../${SD}:/

	@echo "==== ${FS} format sdcard image created  ====\n"
