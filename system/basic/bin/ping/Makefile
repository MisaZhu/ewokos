SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PING_OBJS = ping.o

PING = $(TARGET_DIR)/bin/ping

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PING): $(PING_OBJS) \
		$(EWOK_LIBC_A) \
		$(BUILD_DIR)/lib/libsocket.a
	$(LD) -Ttext=100 $(PING_OBJS) -o $(PING) $(LDFLAGS) -lsocket $(EWOK_LIBC) 

clean:
	rm -f $(PING_OBJS)
