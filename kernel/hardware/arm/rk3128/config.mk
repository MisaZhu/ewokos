QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio

ARCH_CFLAGS = -march=armv7ve

ARCH=v7
BSP=rk3128

#----enable DPI display---
#DPI=yes
#----multi core(SMP)------
SMP=yes
#----kernel console-------
KERNEL_CONSOLE=yes
