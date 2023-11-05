QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio

ARCH_CFLAGS = -march=armv7ve
ARCH=v7

#----PI4-------
#PI4=yes
#----PI4.clockwork-------
CLOCKWORK=yes
#----enable DPI display---
#DPI=yes
#----multi core(SMP)------
SMP=yes
#----kernel console-------
KERNEL_CONSOLE=yes

ifeq ($(CLOCKWORK),yes)
ARCH_CFLAGS += -DPI4 -DCLOCKWORK
QEMU_FLAGS = -M raspi4 -m 1024M -serial mon:stdio
else ifeq ($(PI4),yes)
ARCH_CFLAGS += -DPI4
QEMU_FLAGS = -M raspi4 -m 1024M -serial mon:stdio
else
QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio
endif
