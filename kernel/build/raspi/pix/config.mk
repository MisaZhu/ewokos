QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio

ARCH_CFLAGS = -march=armv7ve

ARCH=arm/v7
BSP=raspi/piX

#----enable DPI display---
#DPI=yes
#----cortex-a7 cpu--------
#CORTEX_A7=yes
#----multi core(SMP)------
SMP=yes
#----kernel console-------
KERNEL_CONSOLE=yes
