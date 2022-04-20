include config.mk

ifeq ($(DEBUG),yes)
LDS = ../raspix/mkos.lds.qemu.S
else
LDS = ../raspix/mkos.lds.S
endif

QEMU_CMD = qemu-system-aarch64
include ../make.rule
