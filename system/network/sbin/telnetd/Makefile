SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TELNETD_OBJS = telnetd.o

TELNETD = $(TARGET_DIR)/sbin/telnetd

$(TELNETD): $(TELNETD_OBJS) \
		$(BUILD_DIR)/lib/libsocket.a \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(TELNETD_OBJS) -o $(TELNETD) $(LDFLAGS)    -lsocket  $(EWOK_LIBC)

clean:
	rm -f $(TELNETD_OBJS)

