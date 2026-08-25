SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

GREP_OBJS = $(ARCH)/grep.o

GREP = $(TARGET_DIR)/bin/grep

$(GREP): $(GREP_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(GREP_OBJS) -o $(GREP) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(GREP_OBJS)

