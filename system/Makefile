KERNEL_DIR=../kernel

all: basic_sys platform
	@echo "all done."

full: basic_sys platform full_sys
	@echo "all done."

extra: basic_sys platform full_sys extra_sys
	@echo "all done."

basic_sys:
	cd basic;make
full_sys:
	cd full;make
extra_sys:
	cd extra;make
platform:
	cd platforms;make

SD = root.ext2
sd: 
	@echo "\n====building ext2 format sdcard image===="
#16M ext2 sd
	dd if=/dev/zero of=${SD} bs=1k count=16k
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
