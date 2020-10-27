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

rom:
	cd romfs;make
	@echo "\n====building kernel romfs  ===="
	build/genfs build/rootfs > $(KERNEL_DIR)/loadinit/romfs/romfs_data.c
	@echo "==== kernel romfs created ===="

SD = build/sd.ext2
sd: 
	cd romfs;make
	@echo "\n====building ext2 format sdcard image===="
#256M ext2 sd
	dd if=/dev/zero of=${SD} bs=1024 count=262144
	mke2fs ${SD}
	mkdir -p tmp
	sudo fuse-ext2 -o force,rw+ ${SD} ./tmp
	sudo cp -r build/rootfs/* ./tmp
	sudo umount ./tmp
	@echo "==== ext2 format sdcard image created  ====\n"
	rm -r tmp

clean:
	cd basic;make clean
	cd platforms;make clean
	cd full;make clean
	rm -fr build
