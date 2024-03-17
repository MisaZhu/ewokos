KERNEL_DIR = $(ROOT_DIR)/../kernel/kernel
KERNEL_H = \
	$(KERNEL_DIR)/include/proto_t.h \
	$(KERNEL_DIR)/include/syscalls.h \
	$(KERNEL_DIR)/include/signals.h \
	$(KERNEL_DIR)/include/interrupt.h \
	$(KERNEL_DIR)/include/sysinfo.h \
	$(KERNEL_DIR)/include/kevent.h \
	$(KERNEL_DIR)/include/procinfo.h

kernel_heads:
	@mkdir -p ${BUILD_DIR}/include
	@mkdir -p ${BUILD_DIR}/lib
	@mkdir -p ${BUILD_DIR}/rootfs
	cp -r $(KERNEL_H) $(BUILD_DIR)/include/

SD = build/root.ext2

sd: 
	@echo "\n====building ext2 format sdcard image===="
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1k count=128k
#make ext2 fs with: block_size=1024 inode_size=128
	mke2fs -b 1024 -I 128 ${SD}

	@cd $(ROOT_DIR)/build/$(HW); \
	find . -type f | e2cp -ap -G0 -O0 -d ../../hardware/arm/${HW}/${SD}:/

	@cd $(ROOT_DIR)/build/rootfs; \
	find . -type f | e2cp -ap -G0 -O0 -d ../../hardware/arm/${HW}/${SD}:/

	@echo "==== ext2 format sdcard image created  ====\n"

