KERNEL_DIR = $(ROOT_DIR)/../kernel/kernel
KERNEL_H = \
	$(KERNEL_DIR)/include/proto_t.h \
	$(KERNEL_DIR)/include/syscalls.h \
	$(KERNEL_DIR)/include/signals.h \
	$(KERNEL_DIR)/include/interrupt.h \
	$(KERNEL_DIR)/include/sysinfo.h \
	$(KERNEL_DIR)/include/kevent.h \
	$(KERNEL_DIR)/include/shmflag.h \
	$(KERNEL_DIR)/include/procinfo.h

kernel_heads:
	@mkdir -p ${BUILD_DIR}/include
	@mkdir -p ${BUILD_DIR}/lib
	cp -r $(KERNEL_H) $(BUILD_DIR)/include/

#SD = $(ROOT_DIR)/root.ext2
SD = build/root.ext2
sd: 
	@echo "\n====building ext2 format sdcard image===="
	sudo rm -fr build/tmp
	mkdir -p build/tmp
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1k count=256k
#make ext2 fs with: block_size=1024 inode_size=128
	mke2fs -b 1024 -I 128 ${SD}

#mkdir tmp dir and mount sdimage to tmp
	sudo fuse-ext2 -o force,rw+ ${SD} build/tmp

#cp dirs and files
	sudo cp -r $(ROOT_DIR)/build/rootfs/* build/tmp

#umount tmp
	sudo umount build/tmp
	@echo "==== ext2 format sdcard image created  ====\n"
	rm -r build/tmp

