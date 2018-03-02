# EwokOS
.Author

	Misa.Z misa.zhu@gmail.com

.About

	A microkernel os for learning operating system. 

.Environment & Tools

	Ubuntu Linux 16.04 with "qemu-system-arm","gcc-arm-none-eabi","gdb-arm-none-eabi" installed(can install by "apt")
	
.Source code read-guide

	boot.S => kalloc.c => mmu.c => proc.c => sheduler.c => syscalls.c => pmalloc.c

	Tips: Don't fall in love with assembly too much;).

.make and run

	"make run": run EwokOS; ("qemu-system-arm -kernel build/EwokOS.bin -initrd build/initfs" boot kernel file and load init ramdisk.)

	"make debug": run EwokOS at debug server-mode;

	"make gdb": debug EwokOS (debug client-mode);
