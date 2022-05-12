include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

LS_OBJS = xmodem.o crc16.o

RX = $(TARGET_DIR)/bin/rx

$(RX): $(LS_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(LS_OBJS) -o $(RX) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(LS_OBJS)

