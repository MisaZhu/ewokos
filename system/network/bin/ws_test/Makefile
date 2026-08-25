SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = $(ARCH)/ws_test.o

TASK = $(TARGET_DIR)/bin/ws_test

$(TASK): $(TASK_OBJS) \
		$(BUILD_DIR)/lib/libwebsockets.a \
		$(BUILD_DIR)/lib/libwolfssl.a \
		$(BUILD_DIR)/lib/libsocket.a \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libopenlibm.a
		$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lwebsockets -lwolfssl -lopenlibm -lsocket $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
