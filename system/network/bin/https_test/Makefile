SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = https_test.o

TASK = $(TARGET_DIR)/bin/https_test

$(TASK): $(TASK_OBJS) \
		$(EWOK_LIBC_A)
		$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -ltinyhttpsc -lsocket $(EWOK_LIBC) 

clean:
	rm -f $(TASK_OBJS)

