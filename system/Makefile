# tools
AR = arm-none-eabi-ar
#AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld

COMMON_LIB=../lib
COMMON_OBJ=$(COMMON_LIB)/src/string.o

# flags
CFLAGS = -gstabs -I. \
				 -I ../kernel/include \
				 -I lib/include \
				 -I $(COMMON_LIB)/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

#ASFLAGS = -g -I $(KERNEL_LIB)/include

all: system

include lib/build.mk
include mkramfs/build.mk
include sbin/build.mk


system: $(MKRAMFS) $(PROGRAM) $(COMMON_OBJ)
	mkdir -p build
	$(MKRAMFS) build/initfs build/sbin

clean:
	rm -f $(EXTRA_CLEAN)
	rm -fr mkramfs/mkramfs.dSYM
	rm -fr build
