#-----raspberry arm config----------
#ARCH		= arm
#ARCH_VER	= v7
#QEMU_MACHINE= raspi2b
#LOAD_ADDRESS= 0x8000

#-----raspberry arm for qemu----------
#ARCH		= arm
#ARCH_VER	= v7
#QEMU_MACHINE= raspi2b
#LOAD_ADDRESS= 0x10000

#-----raspberry aarch64 config----------
ARCH		= aarch64
ARCH_VER	= v8
LOAD_ADDRESS= 0x80000

QEMU_MACHINE= raspi3b

#QEMU_MACHINE= raspi4b
#PI4=yes

#----CLOCKWORK-------
#CLOCKWORK=yes

#----enable DPI display---
#DPI=yes

#----multi core(SMP)------
SMP=yes

