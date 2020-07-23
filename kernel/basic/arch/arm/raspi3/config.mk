CPU = arm1176jzf-s
#QEMU_FLAGS = -cpu $(CPU) -M raspi2 -m 512M -serial mon:stdio -nographic -display none
QEMU_FLAGS = -cpu arm1176 -M raspi3 -m 512M -serial mon:stdio
ARCH_QEMU_CFLAGS = -mcpu=$(CPU) -DA_CORE
