CPU = arm926ej-s
#QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -nographic -display none -serial mon:stdio
QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -serial mon:stdio
ARCH=arm/v6
BSP=versatilepb
#SYS_LOAD_MODE=sd
SYS_LOAD_MODE=romfs
