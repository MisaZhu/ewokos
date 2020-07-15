CPU = cortex-a7
#QEMU_FLAGS = -cpu $(CPU) -M raspi2 -m 512M -serial mon:stdio -nographic -display none
QEMU_FLAGS = -cpu $(CPU) -M raspi2 -m 512M -serial mon:stdio
ARCH_QEMU_CFLAGS = -mcpu=$(CPU) -DRASPI2 -DA_CORE
