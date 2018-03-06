# EwokOS
.Author

	Misa.Z misa.zhu@gmail.com

.About

	A microkernel os for learning operating system. 

.Environment & Tools

	Ubuntu Linux 16.04 with "qemu-system-arm","gcc-arm-none-eabi","gdb-arm-none-eabi"
	installed(can install by "apt")
	
.Source code read-guide

	kernel/arm/boot.S
	kernel/src/kalloc.c
	kernel/src/mmu.c
	kernel/src/proc.c 
	kernel/src/sheduler.c
	kernel/src/syscalls.c
	kernel/src/kmessage.c
	lib/src/pmalloc.c
	lib/src/pmessage.c
	sbin/kserv/kserv.c
	sbin/vfs/vfs.c

	Tips: Don't fall in love with assembly too much;).

.make and run

	"make run": run EwokOS; 
	  "qemu-system-arm -kernel build/EwokOS.bin -initrd build/initfs"
	  boot kernel file and load init ramdisk.

	"make debug": run EwokOS at debug server-mode;
	"make gdb": debug EwokOS (debug client-mode);
