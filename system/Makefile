# tools
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

# flags
CFLAGS = -gstabs -I. \
				 -I ../lib/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

ASFLAGS = -g -I ../lib/include
QEMU_FLAGS = $(ARCH_QEMU_FLAGS) -nographic

all: ../lib/libewok.a system

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
