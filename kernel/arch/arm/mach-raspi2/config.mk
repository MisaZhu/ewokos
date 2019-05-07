#CPU = arm1176jz-s
#QEMU_FLAGS = -M raspi2 -m 256M -cpu arm1176jz-s

CPU = cortex-a7
QEMU_FLAGS = -M raspi2 -m 256M
ARCH_QEMU_CFLAGS = -DMULTI_CPU -DA_CORE
