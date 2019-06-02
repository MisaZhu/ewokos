CPU = cortex-a9
QEMU_FLAGS = -M realview-pbx-a9 -smp 4 -m 256M
ARCH_QEMU_CFLAGS = -DMULTI_CPU -DA_CORE
