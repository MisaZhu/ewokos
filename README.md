# EwokOS
.Author

	Misa.Z misa.zhu@gmail.com

.About

	A microkernel os for learning operating system. 

.Environment & Tools

	Linux:	
		Ubuntu Linux 16.04 with "qemu-system-arm","gcc-arm-none-eabi","gdb-arm-none-eabi"
		installed(can install by "apt")

	Mac OSX(with brew installed):	
		brew tap PX4/homebrew-px4
		brew install gcc-arm-none-eabi-49
		brew install qemu
		(set the right PATH environment after installed)
	
.make and run
	
	make initramfs first, and then make kernel.
	
	"make run": run EwokOS; 
	  "qemu-system-arm -kernel build/EwokOS.bin -initrd ../initramfs/build/initfs"
	  boot kernel file and load init ramdisk.

	"make debug": run EwokOS at debug server-mode;
	"make gdb": debug EwokOS (debug client-mode);

.commands 
	
	Most of commands are in 'system/sbin' directory, like:
	ls, ps, pwd, test ......

.Source code read-guide

	kernel/arm/boot.S
	kernel/src/mm/kalloc.c
	kernel/src/mm/mmu.c
	kernel/src/proc.c 
	kernel/src/sheduler.c
	kernel/src/syscalls.c
	kernel/src/kmessage.c

	system/lib/src/stdlib.c
	system/lib/src/unistd.c
	system/lib/src/pmessage.c

	initramfs/sbin/init/init.c
	initramfs/sbin/vfs/vfs.c
	initramfs/sbin/shell/shell.c

	Tips: Don't fall in love with assembly too much;).

. Kernel init memory map

	PhyMem                         VMem
	0x00000000-0x00080000  map to  0x80000000-0x80080000  (8M)

	PhyMem        VMem         Desc
	----------------------------------------------------
	0x00000000    0xFFFF0000   interrupt table
	0x00010000    0x80010000   Kernel start (load to)
	***           ***          (_initStack, _irqStack, _startupPageDir)
	***           ***          Kernel end, Kernel PageDir Table start
	+16KB         +16KB        Kernel PageDir Table end.
	+32KB         +32KB        kernel malloc base
	+2M           +2M          kernel malloc end (size=2M).
	......
	0x08000000    0x08000000   init ramdisk base (load initrd to)
	0x08010000    0x08010000   init ramdisk end (size=1M).
	......
	MMIO_BASE_PHY MMIO_BASE    MMIO base (arch)
	......


