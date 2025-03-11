CPU = arm926ej-s
#QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -nographic -display none -serial mon:stdio 
QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -serial mon:stdio 
ARCH_CFLAGS = -fno-strict-aliasing -mcpu=$(CPU)  -DARM_V6 -DSCHD_TRACE -DKCONSOLE
ARCH=v5
BSP=ev3
