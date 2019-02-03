# tools
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

KERNEL_LIB=../kernel/lib

# flags
CFLAGS = -gstabs -I. \
				 -I lib/include \
				 -I $(KERNEL_LIB)/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

ASFLAGS = -g -I $(KERNEL_LIB)/include
QEMU_FLAGS = $(ARCH_QEMU_FLAGS) -nographic

all: $(KERNEL_LIB)/libewok.a system

include lib/build.mk
include mkramfs/build.mk
include sbin/build.mk


system: $(MKRAMFS) $(PROGRAM)
	mkdir -p build
	$(MKRAMFS) build/initfs sbin/sbin

clean:
	rm -f $(EXTRA_CLEAN)
	rm -fr mkramfs/mkramfs.dSYM
	rm -fr build
	rm -fr sbin/sbin
