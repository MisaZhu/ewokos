SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

REBOOT_OBJS = $(ARCH)/reboot.o

REBOOT = $(TARGET_DIR)/bin/reboot

$(REBOOT): $(REBOOT_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(REBOOT_OBJS) -o $(REBOOT) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(REBOOT_OBJS)