CPU = cortex-a7
QEMU_FLAGS = -cpu $(CPU) -M raspi2 -m 512M -serial mon:stdio
ARCH_CFLAGS = -march=armv7ve
LOAD_MODE=sd
#LOAD_MODE=romfs
