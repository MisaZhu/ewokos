KERNEL_DIR = $(ROOT_DIR)/../kernel
KERNEL_H = \
	$(KERNEL_DIR)/kernel/include/proto_t.h \
	$(KERNEL_DIR)/kernel/include/syscalls.h \
	$(KERNEL_DIR)/kernel/include/signals.h \
	$(KERNEL_DIR)/kernel/include/interrupt.h \
	$(KERNEL_DIR)/kernel/include/sysinfo.h \
	$(KERNEL_DIR)/kernel/include/kevent.h \
	$(KERNEL_DIR)/kernel/include/procinfo.h \
	$(KERNEL_DIR)/platform/arm/arch/common/include/arch_context.h

kernel_heads:
	@mkdir -p ${BUILD_DIR}/include
	@mkdir -p ${BUILD_DIR}/lib
	@mkdir -p ${BUILD_DIR}/rootfs
	cp -r $(KERNEL_H) $(BUILD_DIR)/include/

SD = build/root.ext2

sd: 
	@echo "\n====building ext2 format sdcard image===="
	mkdir -p build
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1k count=128k
#make ext2 fs with: block_size=1024 inode_size=128
	mke2fs -b 1024 -I 128 ${SD}

	@cd $(ROOT_DIR)/build/$(HW); \
	find . -type f | e2cp -ap -G0 -O0 -d ../../platform/arm/${HW}/${SD}:/

	@mkdir -p $(ROOT_DIR)/build/extra; \
	cd $(ROOT_DIR)/build/extra; \
	find . -type f | e2cp -ap -G0 -O0 -d ../../platform/arm/${HW}/${SD}:/

	@echo "==== ext2 format sdcard image created  ====\n"

