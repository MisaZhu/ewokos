include config.mk

ifeq ($(DEBUG),yes)
LDS = ../raspix/mkos.lds.raspi1.qemu.S
else
LDS = ../raspix/mkos.lds.raspi1.S
endif

QEMU_CMD = qemu-system-arm
include ../make.rule
