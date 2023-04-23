QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio

ARCH_CFLAGS = -march=armv8-a

ARCH=v7
BSP=rk3326

#----enable DPI display---
#DPI=yes
#----multi core(SMP)------
SMP=no
#----kernel console-------
KERNEL_CONSOLE=yes
