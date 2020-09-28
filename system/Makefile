KERNEL_DIR=../kernel

all: tiny_sys kernelfs
	@echo "all done."

full: tiny_sys extra_sys kernelfs
	@echo "all done."

tiny_sys:
	cd tiny;make
extra_sys:
	cd extra;make

kernelfs:
	cd romfs;make
	@echo "\n====building kernel romfs  ===="
	cp build/rootfs/drivers/romfsd build/rootfs/drivers/rootfsd
	build/genfs build/rootfs > $(KERNEL_DIR)/loadinit/romfs_data.c
	@echo "==== kernel romfs created ===="

SD = build/sd.ext2
sd: tiny_sys extra_sys
	cp build/rootfs/drivers/sdfsd build/rootfs/drivers/rootfsd
	@echo "\n====building ext2 format sdcard image===="
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1024 count=262144
	mke2fs ${SD}
	mkdir -p tmp
	sudo fuse-ext2 -o force,rw+ ${SD} ./tmp
	sudo cp -r build/rootfs/* ./tmp
	sudo umount ./tmp
	@echo "==== ext2 format sdcard image created  ====\n"
	@echo "====building kernel romfs  ===="
	mkdir -p tmp/sbin
	mkdir -p tmp/drivers
	cp build/rootfs/sbin/init tmp/sbin
	cp build/rootfs/sbin/procd tmp/sbin
	cp build/rootfs/sbin/vfsd tmp/sbin
	cp build/rootfs/drivers/rootfsd tmp/drivers
	build/genfs tmp > $(KERNEL_DIR)/loadinit/romfs_data.c
	rm -r tmp
	@echo "==== kernel romfs created ====\n"

clean:
	cd extra;make clean
	cd tiny;make clean
