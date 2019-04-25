# tools
AR = arm-none-eabi-ar
#AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld

COMMON_LIB=../lib
COMMON_OBJ=$(COMMON_LIB)/src/kstring.o \
	$(COMMON_LIB)/src/device.o \
	$(COMMON_LIB)/src/ext2.o \
	$(COMMON_LIB)/src/vprintf.o

# flags
CFLAGS = -I. -g \
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
