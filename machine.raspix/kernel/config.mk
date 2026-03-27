ifeq ($(ARCH),)
ARCH = aarch64
endif

#-----raspberry arm config----------
ifeq ($(ARCH),arm)
ARCH		  = arm
ARCH_VER	  = v7
QEMU_MACHINE  = raspi2b
#LOAD_ADDRESS  = 0x10000 #for qemu
LOAD_ADDRESS = 0x8000 #for real hw
#-----raspberry aarch64 config----------
else
ARCH		  = aarch64
ARCH_VER	  = v8
LOAD_ADDRESS  = 0x80000
QEMU_MACHINE  = raspi3b
endif

#----multi core(SMP)------
SMP=yes
