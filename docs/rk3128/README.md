## How to create bootable sdcard  for rk3128


### STEP 1: prepare idbloader.img、uboot.img 

NOTE: I put this two bin file in this directory. it was tested  on my hardware.
	  so you can jump to SETP 2 ^_^

#### 1.use rkbin tool to create idbloader.img.  

download tool form: https://github.com/rockchip-linux/rkbin.git

run below command in rkbin  directory to create idbloader for rk3128:

	tools/mkimage -n rk3128 -T rksd -d  bin/rk31/rk3128_ddr_300MHz_v2.12.bin idbloader.img
	cat bin/rk31/rk3128x_miniloader_v2.57.bin >> idbloader.img

#### 2.use u-boot to build uboot.img

download rockchip u-boot source form: http://github.com/tonytsangzen/u-boot-rk3128-gamepad.git

this repo is fork from:  https://github.com/rockchip-linux/u-boot.git

I do some modify for rk3128 gamepad

after download u-boot source, follow u-boot instruction to cratet build environment

then run build command:

	./make.sh rk3128k


### STEP 2: create bootable sdcard

rockchip bootable sdcard is  organized as：

	--------------------    -----
	MBR/GPT         32KB
	--------------------    8MB	
	IDBLOADER     8160KB
	--------------------    -----
	UBOOT            4MB
	--------------------
	TRUST            4MB    120MB
	--------------------
	KERNEL         112MB	
	--------------------    -----
	ROOTFS
	......
	--------------------    -----

use below commond to write idbloader.img & uboot.img:

	sudo dd if=idbloader.img of=/dev/disk? seek=64 bs=512
	sudo dd if=uboot.img of=/dev/disk? seek=16384 bs=512

IMPOTENT: pleace replace "disk?" to your own device name.

then use fdisk to format sdcard, create first partion form 128MB.

please follow EwokOS/README to create boot & rootfs partion.

more infomation see: 

   [Rockchip Wiki: Boot option](https://opensource.rock-chips.com/wiki_Boot_option)
