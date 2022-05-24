QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio

ARCH_CFLAGS = -march=armv7ve -DPI4

ARCH=arm/v7
BSP=raspi/pix

#----enable DPI display---
#DPI=yes
#----multi core(SMP)------
SMP=yes
#----kernel console-------
KERNEL_CONSOLE=yes
