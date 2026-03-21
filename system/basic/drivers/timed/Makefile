SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = timed.o

TASK = $(TARGET_DIR)/drivers/timed

$(TASK): $(TASK_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lntpc -lsocket $(EWOK_LIBC)

clean:
	rm -f $(TASK)
	rm -f $(TASK_OBJS)
