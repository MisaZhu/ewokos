KERNEL_DIR=../kernel
TARGET_DIR = build/rootfs

all: basic_sys
	@echo "all done."

full: basic_sys full_sys
	@echo "all done."

extra: basic_sys full_sys extra_sys
	@echo "all done."

basic_sys:
	cd basic;make
	cd bsp/basic;make
full_sys:
	cd full;make
	cd bsp/full;make
extra_sys:
	cd extra;make

SD = root.ext2
sd: 
	@echo "\n====building ext2 format sdcard image===="
#16M ext2 sd
	dd if=/dev/zero of=${SD} bs=1k count=16k
#make ext2 fs with: block_size=1024 inode_size=128
	mke2fs -b 1024 -I 128 ${SD}

#mkdir tmp dir and mount sdimage to tmp
	mkdir -p tmp
	sudo fuse-ext2 -o force,rw+ ${SD} ./tmp

#cp dirs and files
	sudo cp -r build/rootfs/* ./tmp

#umount tmp
	sudo umount ./tmp
	@echo "==== ext2 format sdcard image created  ====\n"
	rm -r tmp

clean:
	cd basic;make clean
	cd bsp/basic;make clean
	cd full;make clean
	cd bsp/full;make clean
	rm -fr build
