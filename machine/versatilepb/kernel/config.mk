CPU = arm926ej-s
#QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -nographic -display none -serial mon:stdio 

QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -serial mon:stdio -display cocoa

ifeq ($(QEMU_DISPLAY_OPTS),cocoa)
QEMU_FLAGS += -display cocoa
endif


ARCH_CFLAGS = -mcpu=$(CPU) -DARM_V6
ARCH_VER=v6
BSP=versatilepb
