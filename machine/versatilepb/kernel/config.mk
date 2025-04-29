CPU = arm926ej-s
#QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -nographic -display none -serial mon:stdio 
QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -serial mon:stdio

ARCH_CFLAGS = -mcpu=$(CPU) -DARM_V6
ARCH_VER=v6
BSP=versatilepb
