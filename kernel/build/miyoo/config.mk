CPU = cortex-a7
QEMU_FLAGS = -cpu $(CPU) -M raspi2b -m 128M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve -DSKIP_HYP -DDUAL_CORE -DCORTEX_A7
ARCH=arm/v7
BSP=miyoo
SMP=yes