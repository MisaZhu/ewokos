AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

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

CFLAGS += -I $(TARGET_DIR)/include

TGA_OBJS = tga.o

TGA = $(TARGET_DIR)/bin/tga

$(TGA): $(TGA_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a \
		$(TARGET_DIR)/lib/libtga.a \
		$(TARGET_DIR)/lib/libx.a \
		$(TARGET_DIR)/lib/libgraph.a
	$(LD) -Ttext=100 $(TGA_OBJS) -o $(TGA) $(LDFLAGS) -lgraph -lx -ltga -lewokc -lc

clean:
	rm -f $(TGA_OBJS)

