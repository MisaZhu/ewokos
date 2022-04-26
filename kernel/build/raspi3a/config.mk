CPU = cortex-a53
QEMU_FLAGS = -cpu $(CPU) -M raspi3ap -m 512M -serial mon:stdio
ARCH_CFLAGS = -mcpu=cortex-a53 -DPI3A 
ARCH=arm/v7
BSP=raspix
SMP=yes
