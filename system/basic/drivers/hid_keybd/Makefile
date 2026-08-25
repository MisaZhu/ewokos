SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_KEYBD_OBJS = $(ARCH)/hid_keybd.o

HID_KEYBD = $(TARGET_DIR)/drivers/$(HW)/hid_keybd

$(HID_KEYBD): $(HID_KEYBD_OBJS) \
                $(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(LD) -Ttext=100 $(HID_KEYBD_OBJS) -o $(HID_KEYBD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_KEYBD_OBJS)
