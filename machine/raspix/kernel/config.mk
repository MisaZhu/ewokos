ARCH_CFLAGS = -march=armv7ve
ARCH_VER=v7

#----PI4-------
PI4=yes
#----CLOCKWORK-------
#CLOCKWORK=yes
#----enable DPI display---
#DPI=yes
#----multi core(SMP)------
SMP=yes

ifeq ($(CLOCKWORK),yes)
PI4=yes
ARCH_CFLAGS += -DCLOCKWORK
endif

ifeq ($(PI4),yes)
#QEMU_CMD = qemu-system-aarch64
QEMU_CMD = qemu-system-arm
ARCH_CFLAGS += -DPI4
QEMU_FLAGS = -M raspi4b -m 2048M -serial mon:stdio 
else
QEMU_CMD = qemu-system-arm
QEMU_FLAGS = -M raspi2b -m 1024M -serial mon:stdio -display cocoa
endif
