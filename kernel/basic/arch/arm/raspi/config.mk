CPU = arm1176jzf-s
#QEMU_FLAGS = -cpu arm1176 -M raspi -m 512M -serial mon:stdio -nographic -display none
QEMU_FLAGS = -cpu arm1176 -M raspi -m 512M -serial mon:stdio
ARCH_QEMU_CFLAGS = -mcpu=$(CPU) -DRASPI
