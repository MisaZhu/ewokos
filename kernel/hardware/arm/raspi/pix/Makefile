include config.mk

LDS_QEMU = mkos.lds.qemu.S
LDS = mkos.lds.S


QEMU_CMD = qemu-system-arm

ROOT_DIR = ../../../../..
MACHINE = arm/raspix

ifeq ($(PI4),yes)

OS_IMG = kernel8.img
OS_ELF = kernel8.elf
OS_QEMU_IMG = kernel8.qemu.img
OS_QEMU_ELF = kernel8.qemu.elf
include make.bsp.pi4

else

OS_IMG = kernel7.img
OS_ELF = kernel7.elf
OS_QEMU_IMG = kernel7.qemu.img
OS_QEMU_ELF = kernel7.qemu.elf
include make.bsp

endif

include ../../make.rule
