SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

LS_OBJS = xmodem.o crc16.o

RX = $(TARGET_DIR)/bin/rx

$(RX): $(LS_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(LS_OBJS) -o $(RX) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(LS_OBJS)

