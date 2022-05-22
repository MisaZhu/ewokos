CPU = arm1176jzf-s
QEMU_FLAGS = -cpu arm1176 -M raspi0 -m 512M -serial mon:stdio
ARCH_CFLAGS = -mcpu=$(CPU)
ARCH=arm/v6
BSP=raspi

#----enable DPI display---
#DPI=yes
#----cortex-a7 cpu--------
#CORTEX_A7=yes
#----multi core(SMP)------
SMP=yes
#----kernel console-------
KERNEL_CONSOLE=yes

