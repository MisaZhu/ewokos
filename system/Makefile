all: basic_fs kernelfs
	@echo "all done."

full: basic_fs extra_fs kernelfs
	@echo "all done."

basic_fs:
	cd basic;make
extra_fs:
	cd extra;make

SD = build/sd.ext2
sd:
	@echo "\n====building ext2 format sdcard image===="
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1024 count=262144
	mke2fs ${SD}
	mkdir -p tmp
	sudo fuse-ext2 -o force,rw+ ${SD} ./tmp
	sudo cp -r build/rootfs/* ./tmp
	sudo umount ./tmp
	rm -r tmp
	@echo "==== ext2 format sdcard image created  ====\n"

kernelfs:
	cd kfs;make

KERNEL_DIR=../kernel/basic
ARCH_DIR=$(KERNEL_DIR)/arch
KERNEL=$(KERNEL_DIR)/build/mkos.bin
KERNEL_ELF=$(KERNEL_DIR)/build/mkos.elf

clean:
	cd extra;make clean
	cd basic;make clean
