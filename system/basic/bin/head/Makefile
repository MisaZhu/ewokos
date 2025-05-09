SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

head_OBJS = head.o

head = $(TARGET_DIR)/bin/head

$(head): $(head_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(head_OBJS) -o $(head) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(head_OBJS)

