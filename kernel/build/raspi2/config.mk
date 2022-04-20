CPU = cortex-a7
QEMU_FLAGS = -cpu $(CPU) -M raspi2b -m 1024M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve -DKERNEL_SMP -DPI2 -DUIDIV
ARCH=arm/v7
BSP=raspix
