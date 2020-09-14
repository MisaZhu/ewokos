1.download raspbian lite img file from https://www.raspberrypi.org/downloads/raspbian/

2.restore the img to a microsd card(size >= 4G)

3.format the ext4 partition with ext2 format by this command:

    sudo mke2fs -L rootfs -b 1024 -I 128 /dev/(SD_PARTITION_EXT4)

4.cd system and make

5.copy system/build/rootfs/* to ext2 partition root dir

6.remove  kernel*.img files from boot partition root dir.

7.cd build path and remake kernel (with make clean)

8.copy kernel image file(kernel.img / kernel7.img) to boot partition
