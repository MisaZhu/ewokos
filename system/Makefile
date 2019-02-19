# tools
AR = arm-none-eabi-ar
#AS = arm-none-eabi-as
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld

COMMON_LIB=../lib
COMMON_OBJ=$(COMMON_LIB)/src/kstring.o $(COMMON_LIB)/src/tree.o

# flags
CFLAGS = -I. \
				 -I ../lib/include \
				 -I lib/include \
				 -I $(COMMON_LIB)/include \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -g \
				 -std=c99 

#ASFLAGS = -g -I $(KERNEL_LIB)/include

all: system

include lib/build.mk
include mkramfs/build.mk
include sbin/build.mk
include kserv/build.mk


system: $(MKRAMFS) $(PROGRAM) $(COMMON_OBJ)
	mkdir -p build
	$(MKRAMFS) ../kernel/include/initfs.h build

clean:
	rm -f $(EXTRA_CLEAN) $(COMMON_OBJ) ../kernel/include/initfs.h 
	rm -fr mkramfs/mkramfs.dSYM
	rm -fr build
