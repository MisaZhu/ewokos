all: tiny_sys kernelfs
	@echo "all done."

full: tiny_sys extra_sys kernelfs
	@echo "all done."

tiny_sys:
	cd tiny;make
extra_sys:
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

kinit:
	cd kfs;make init

KERNEL_DIR=../kernel/tiny
ARCH_DIR=$(KERNEL_DIR)/arch
KERNEL=$(KERNEL_DIR)/build/mkos.bin
KERNEL_ELF=$(KERNEL_DIR)/build/mkos.elf

clean:
	cd extra;make clean
	cd tiny;make clean
