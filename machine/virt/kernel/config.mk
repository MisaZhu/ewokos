ifeq ($(ARCH),)
ARCH = aarch64
endif

#-----virt arm config----------
ifeq ($(ARCH),arm)
ARCH		  = arm
ARCH_VER	  = v7
QEMU_MACHINE  = virt
#-----virt aarch64 config----------
else
ARCH		  = aarch64
ARCH_VER	  = v8
QEMU_MACHINE  = virt
endif

LOAD_ADDRESS  = 0x40080000

#----multi core(SMP)------
SMP=yes
