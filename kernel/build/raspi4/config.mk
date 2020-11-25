CPU = cortex-a7
QEMU_FLAGS = -cpu $(CPU) -M raspi2 -m 1024M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve -DKERNEL_SMP
ARCH=arm/v7
BSP=raspi4
SYS_LOAD_MODE=sd
#SYS_LOAD_MODE=romfs
