format the partition with ext2/ext3 format by this command:

    sudo mke2fs -L rootfs -b 1024 -I 128 /dev/(SD_PARTITION_EXT4)
		or:
    sudo mke2fs -L rootfs -b 4096 -I 128 /dev/(SD_PARTITION_EXT4)
		or:
    sudo mke2fs -t ext3 -L rootfs -b 1024 -I 128 /dev/(SD_PARTITION_EXT4)
		or:
    sudo mke2fs -t ext3 -L rootfs -b 4096 -I 128 /dev/(SD_PARTITION_EXT4)
