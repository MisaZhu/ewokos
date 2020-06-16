AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

# flags
CFLAGS = -Wstrict-prototypes \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

#CFLAGS +=  -g
CFLAGS +=  -O2

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_M_OBJS = src/mstrx.o \
	src/marray.o \
	src/mutf8.o

LIB_M = $(TARGET_DIR)/lib/libm.a

$(LIB_M): $(LIB_M_OBJS)
	$(AR) rT $(LIB_M) $(LIB_M_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_M_OBJS)
	rm -f $(LIB_M)

