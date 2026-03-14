SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

TELNET_OBJS = telnet.o

TELNET = $(TARGET_DIR)/bin/telnet

$(TELNET): $(TELNET_OBJS) \
		$(EWOK_LIBC_A)
		$(LD) -Ttext=100 $(TELNET_OBJS) -o $(TELNET) $(LDFLAGS) -lsocket $(EWOK_LIBC) 

clean:
	rm -f $(TELNET_OBJS)
