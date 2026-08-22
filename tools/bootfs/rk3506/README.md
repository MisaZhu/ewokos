## How to create bootable sdcard  for rk3506


### STEP 1: create gpt table 

rockchip bootable sdcard is  organized as：

	--------------------    -----
	MBR/GPT         32KB
	--------------------    4MB	
	IDBLOADER     4064KB
	--------------------    -----
	UBOOT            4MB
	--------------------     
	KERNEL         256MB	
	--------------------    -----
	ROOTFS
	......
	--------------------    -----

    dd if=gpt.bin of=/dev/sd?

### STEP 2: write uboot to UBOOT partition
    
    dd if=uboot.img of=/dev/sd?1

### STEP 3:

    follow EwokOS/README to create boot & rootfs partion.

more infomation see: 

   [Rockchip Wiki: Boot option](https://opensource.rock-chips.com/wiki_Boot_option)
