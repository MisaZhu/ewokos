CPU = arm1176jzf-s
#QEMU_FLAGS = -cpu arm1176 -M raspi -m 512M -serial mon:stdio -nographic -display none
QEMU_FLAGS = -cpu arm1176 -M versatilepb -m 256M -serial mon:stdio
ARCH_CFLAGS = -mcpu=$(CPU) 
SYS_LOAD_MODE=sd
#SYS_LOAD_MODE=romfs
