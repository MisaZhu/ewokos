include config.mk

LDS_QEMU = mkos.lds.qemu.S
LDS = mkos.lds.S

OS_IMG = kernel.img
OS_ELF = kernel.elf
OS_QEMU_IMG = kernel.qemu.img
OS_QEMU_ELF = kernel.qemu.elf

QEMU_CMD = qemu-system-arm

ROOT_DIR = ../../../../..
CFLAGS = -I../lib/bcm283x/include
ARCH_OBJS += ../lib/bcm283x/src/gpio.o \
	../lib/bcm283x/src/sd.o \
	../lib/bcm283x/src/mailbox.o \
	../lib/bcm283x/src/pl011_uart.o \
	../lib/bcm283x/src/mini_uart.o \
	../lib/bcm283x/src/board.o \
	../lib/bcm283x/src/framebuffer.o
include ../../make.rule
