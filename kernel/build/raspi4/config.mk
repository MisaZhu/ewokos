CPU = cortex-a53
QEMU_FLAGS = -cpu $(CPU) -M raspi3 -m 1024M -serial mon:stdio
ARCH_CFLAGS = -mcpu=cortex-a53 -DKERNEL_SMP
#ARCH_CFLAGS =  -mcpu=cortex-a53
ARCH=arm/v7
BSP=raspi3
SYS_LOAD_MODE=sd
#SYS_LOAD_MODE=romfs
