CPU = cortex-a7
QEMU_FLAGS = -cpu $(CPU) -M raspi2b -m 128M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve
ARCH=v7

#----enable DPI display---
#DPI=yes
#----multi core(SMP)------
SMP=yes
#----kernel console-------
KERNEL_CONSOLE=yes
