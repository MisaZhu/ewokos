CPU = rv64
QEMU_FLAGS = -cpu $(CPU) -nographic -M virt -smp 1 -m 2G -bios default -serial mon:stdio
QEMU_FLAGS += -device loader,file=../../../system/root.ext2,addr=0xe0000000 
ARCH_CFLAGS = -I ../../../kernel/basic/arch/sys/riscv/common/include  
ARCH=riscv/rv64
BSP=virt

#----enable DPI display---
#DPI=yes
#----multi core(SMP)------
SMP=no
#----kernel console-------
KERNEL_CONSOLE=no

DEBUG=yes
