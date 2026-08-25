SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_TOUCHD_OBJS = $(ARCH)/hid_touchd.o

HID_TOUCHD = $(TARGET_DIR)/drivers/$(HW)/hid_touchd

$(HID_TOUCHD): $(HID_TOUCHD_OBJS) \
                $(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(LD) -Ttext=100 $(HID_TOUCHD_OBJS) -o $(HID_TOUCHD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_TOUCHD_OBJS)
