QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve -DKCONSOLE
ARCH=arm/v7
BSP=raspix

#CORTEX_A7=yes
SMP=yes

