CPU = cortex-a7
QEMU_FLAGS = -cpu $(CPU) -M raspi2 -m 512M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve -DPI3A -DKERNEL_SMP
ARCH=arm/v7
BSP=raspi3
SYS_LOAD_MODE=sd
#SYS_LOAD_MODE=romfs
