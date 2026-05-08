SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = polltest.o

TASK = $(TARGET_DIR)/bin/polltest

$(TASK): $(TASK_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
		$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lsocket $(EWOK_LIBC) 

clean:
	rm -f $(TASK_OBJS)

