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

MINI_UARTD_OBJS = mini_uartd.o

MINI_UARTD = $(TARGET_DIR)/drivers/raspix/mini_uartd

$(MINI_UARTD): $(MINI_UARTD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(MINI_UARTD_OBJS) -o $(MINI_UARTD) $(LDFLAGS) -larch_bcm283x -lewokc -lc

clean:
	rm -f $(MINI_UARTD_OBJS) $(MINI_UARTD)
