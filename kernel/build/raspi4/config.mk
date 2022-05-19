CPU = cortex-a53
QEMU_FLAGS = -cpu $(CPU) -M raspi3 -m 1024M -serial mon:stdio
ARCH_CFLAGS = -mcpu=cortex-a53 -DPI4 -DKCONSOLE
ARCH=arm/v7
BSP=raspix
SMP=yes
