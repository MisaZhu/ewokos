1.copy stage3.bin to boot partition

2.add two line to config.txt
	initramfs kernel7.img 0x2000000
    kernel=stage3.bin

3.follow top readme.txt

