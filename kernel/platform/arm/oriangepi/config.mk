QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio

ARCH_CFLAGS = -march=armv7ve
ARCH=v7

SMP=no
KERNEL_CONSOLE=no
QEMU_CMD = qemu-system-arm
QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio -display cocoa
