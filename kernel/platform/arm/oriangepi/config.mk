QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio

ARCH_CFLAGS = -march=armv7ve -DSCHD_TRACE
ARCH=v7

SMP=no
KERNEL_CONSOLE=no
QEMU_CMD = qemu-system-arm
QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio -display cocoa
