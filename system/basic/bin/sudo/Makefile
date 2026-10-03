SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include -I $(SYS_DIR)/network/libs/utils/include

SUDO_OBJS = $(ARCH)/sudo.o

SUDO = $(TARGET_DIR)/bin/sudo

$(SUDO): $(SUDO_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(SUDO_OBJS) -o $(SUDO) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SUDO_OBJS)
