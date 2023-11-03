include config.mk

LDS_QEMU = mkos.lds.qemu.S
LDS = mkos.lds.S

OS_IMG = kernel7.img
OS_ELF = kernel7.elf
OS_QEMU_IMG = kernel7.qemu.img
OS_QEMU_ELF = kernel7.qemu.elf

QEMU_CMD = qemu-system-arm

ROOT_DIR = ../../../../..
MACHINE = arm/raspix

ifeq ($(CLOCKWORK),yes)
include make.bsp.clockwork
else ifeq ($(PI4),yes)
include make.bsp.pi4
else
include make.bsp
endif

include ../../make.rule
