SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

TEST_OBJS = test.o

TEST = $(TARGET_DIR)/bin/test

$(TEST): $(TEST_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
		$(LD) -Ttext=100 $(TEST_OBJS) -o $(TEST) $(LDFLAGS) -lsocket $(EWOK_LIBC) 

clean:
	rm -f $(TEST_OBJS)

