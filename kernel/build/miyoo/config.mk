CPU = cortex-a7
QEMU_FLAGS = -cpu $(CPU) -M raspi2b -m 128M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve 
ARCH=arm/v7
BSP=miyoo
SMP=yes
BSP_START=yes
