# EwokOS
.Author

	Misa.Z misa.zhu@gmail.com

.About

	A microkernel os for learning operating system. 

.Environment & Tools

	Ubuntu Linux 16.04 with "qemu-system-arm" "gcc-arm-none-eabi" installed(can install by "apt")
	
.Source code read-guide

	boot.S => kalloc.c => mmu.c => proc.c => sheduler.c => syscalls.c

	Tips: Don't fall in love with assembly too much;).
