KERNEL_DIR=../kernel

all: basic_sys platform rom
	@echo "all done."

full: basic_sys platform full_sys rom
	@echo "all done."

basic_sys:
	cd basic;make
full_sys:
	cd full;make
platform:
	cd platforms;make
sd_sys:
	cd platforms/sd;make

rom:
	cd romfs;make
	@echo "\n====building kernel romfs  ===="
	cp build/rootfs/drivers/romfsd build/rootfs/drivers/rootfsd
	build/genfs build/rootfs > $(KERNEL_DIR)/loadinit/romfs_data.c
	@echo "==== kernel romfs created ===="

SD = build/sd.ext2
sd: sd_sys
	cd romfs;make
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
	cp build/rootfs/drivers/rootfsd tmp/drivers
	build/genfs tmp > $(KERNEL_DIR)/loadinit/romfs_data.c
	rm -r tmp
	@echo "==== kernel romfs created ====\n"

clean:
	cd basic;make clean
	cd platforms;make clean
	cd platforms/sd;make clean
	cd full;make clean
	rm -fr build
