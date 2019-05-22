# tools
AR = arm-none-eabi-ar
#AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld

COMMON_LIB=../kernel
COMMON_OBJ=$(COMMON_LIB)/lib/kstring.o \
	$(COMMON_LIB)/lib/basic_math.o \
	$(COMMON_LIB)/lib/device.o \
	$(COMMON_LIB)/lib/sconf_parse.o \
	$(COMMON_LIB)/lib/trunk.o \
	$(COMMON_LIB)/lib/tstr.o \
	$(COMMON_LIB)/lib/vprintf.o \
	$(COMMON_LIB)/arch/arm/common/memcpy.o

# flags
CFLAGS = -I. -g \
			   -Wstrict-prototypes \
				 -I ../lib/include \
				 -I lib/include \
				 -I $(COMMON_LIB)/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 
LDFLAGS = -lgcc

#ASFLAGS = -g -I $(KERNEL_LIB)/include

all: mkdir system

mkdir:
	mkdir -p build/sbin/dev
	mkdir -p build/bin
	mkdir -p build/usr/bin

include lib/build.mk
include dev/build.mk
include sbin/build.mk
include bin/build.mk
include test/build.mk


system: $(PROGRAM) $(COMMON_OBJ)

clean:
	rm -f $(EXTRA_CLEAN) $(COMMON_OBJ)
	rm -fr build
