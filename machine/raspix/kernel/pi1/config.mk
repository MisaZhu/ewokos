CPU = arm1176jzf-s
QEMU_FLAGS = -cpu arm1176 -M raspi0 -m 512M -serial mon:stdio
ARCH_CFLAGS = -mcpu=$(CPU)
ARCH_VER=v6

#----enable DPI display---
#DPI=yes

