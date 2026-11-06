SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_TOUCHPADD_OBJS = $(ARCH)/hid_touchpadd.o

HID_TOUCHPADD = $(TARGET_DIR)/drivers/$(HW)/hid_touchpadd

$(HID_TOUCHPADD): $(HID_TOUCHPADD_OBJS) \
                $(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(LD) -Ttext=100 $(HID_TOUCHPADD_OBJS) -o $(HID_TOUCHPADD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_TOUCHPADD_OBJS)
