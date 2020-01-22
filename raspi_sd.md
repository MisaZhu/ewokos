1.download raspbian lite img file from https://www.raspberrypi.org/downloads/raspbian/

2.restore the img to a microsd card(size >= 4G)

3.format the ext4 partition with ext2 format by this command:

    sudo mke2fs -b 1024 -I 128 /dev/(SD_PARTITION_EXT4)

4.remove  kernel*.img files from boot partition root dir.

5.export MACH=raspi and remake kernel (with make clean)

6.copy kernel/build/mkos.bin to boot partition /kernel.img

7.cd system and make

8.copy system/build/rootfs/* to ext2 partition root dir
