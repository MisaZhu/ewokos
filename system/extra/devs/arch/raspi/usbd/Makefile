AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

BUILD_DIR = ../../../../../build
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

CFLAGS += -I $(TARGET_DIR)/include -I../include
#CFLAGS +=  -g
CFLAGS +=  -O2

#LDFLAGS = -L$(TARGET_DIR)/lib -lnosys -L../lib -lcsud
LDFLAGS = -L$(TARGET_DIR)/lib -lnosys -L../lib -lcsud_debug

USBD_OBJS = usbd.o

USBD = $(TARGET_DIR)/sbin/dev/raspi/usbd

$(USBD): $(USBD_OBJS) \
		$(TARGET_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(USBD_OBJS) -o $(USBD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(USBD_OBJS)

