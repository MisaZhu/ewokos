AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

BUILD_DIR = ../../../../../../build
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

LDFLAGS = -L$(BUILD_DIR)/lib -Llib -lnosys

CFLAGS += -I $(BUILD_DIR)/include -Iinclude -DTARGET_RPI2

USBD_OBJS = usbd.o

USBD = $(TARGET_DIR)/drivers/arm/raspi3/usbd

$(USBD): $(USBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(USBD_OBJS) -o $(USBD) $(LDFLAGS) -larch_bcm283x -lewokc -lcsud -lc

clean:
	rm -f $(USBD_OBJS)
