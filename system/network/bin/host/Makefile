SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

HOST_OBJS = host.o

HOST = $(TARGET_DIR)/bin/host

$(HOST): $(HOST_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
		$(LD) -Ttext=100 $(HOST_OBJS) -o $(HOST) $(LDFLAGS) -lntpc -lsocket $(EWOK_LIBC) 

clean:
	rm -f $(HOST_OBJS)

