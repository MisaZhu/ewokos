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

BUILD_DIR = $(SYS_ROOT_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

HW_KERNEL_DIR = ../kernel

export BSP_LFLAGS = -lbsp -larch_virt

# default to parallel build; sub-makes share the jobserver via $(MAKE)
ifeq ($(MAKELEVEL),0)
ifeq ($(filter -j%,$(MAKEFLAGS)),)
NPROCS := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
MAKEFLAGS += -j$(NPROCS)
endif
endif

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

basic_sys: kernel_heads ewok_heads
	rm -f $(BUILD_DIR)/lib/libbsp.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(MAKE) -C libs
	$(MAKE) -C $(SYS_ROOT_DIR)/basic
	$(MAKE) -C drivers basic
	@mkdir -p ${TARGET_DIR}/etc
	@cp -Rp etc/basic/. ${TARGET_DIR}/etc/

network_sys: basic_sys
	$(MAKE) -C $(SYS_ROOT_DIR)/network
	$(MAKE) -C drivers network
	@mkdir -p ${TARGET_DIR}/etc
	@cp -Rp etc/network/. ${TARGET_DIR}/etc/

gui_sys: network_sys
	$(MAKE) -C $(SYS_ROOT_DIR)/gui
	$(MAKE) -C drivers gui
	@mkdir -p ${TARGET_DIR}/etc
	@cp -Rp etc/gui/. ${TARGET_DIR}/etc/

x_sys: gui_sys
	$(MAKE) -C $(SYS_ROOT_DIR)/xwin
	@mkdir -p ${TARGET_DIR}/etc
	@cp -Rp etc/xwin/. ${TARGET_DIR}/etc/

kernel:
	$(MAKE) -C $(HW_KERNEL_DIR)

kclean:
	$(MAKE) -C $(HW_KERNEL_DIR) clean

run:
	$(MAKE) -C $(HW_KERNEL_DIR) run

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
	rm -fr ${SD}
	dd if=/dev/zero of=${SD} bs=1k count=4096k
#make ext2/ext3 fs with: block_size=4096 inode_size=128
	mke2fs -t ${FS} -b 4096 -I 128 ${SD}

	cd $(FINAL_DIR); \
	find . -type f | e2cp -ap -G0 -O0 -d ../${SD}:/

	@echo "==== ${FS} format sdcard image created  ====\n"
