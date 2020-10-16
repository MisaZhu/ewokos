AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

BUILD_DIR = ../../../../build
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

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

PS2KEYBD_OBJS = ps2keybd.o

PS2KEYBD = $(TARGET_DIR)/drivers/versatilepb/ps2keybd

$(PS2KEYBD): $(PS2KEYBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(PS2KEYBD_OBJS) -o $(PS2KEYBD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(PS2KEYBD_OBJS)
