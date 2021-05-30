CPU = cortex-a53
QEMU_FLAGS = -cpu $(CPU) -M raspi2 -m 512M -serial mon:stdio
ARCH_CFLAGS = -mcpu=cortex-a53 -DPI3A -DKERNEL_SMP
#ARCH_CFLAGS =  -mcpu=cortex-a53 -DPI3A
ARCH=arm/v7
BSP=raspi3
SYS_LOAD_MODE=sd
#SYS_LOAD_MODE=romfs
