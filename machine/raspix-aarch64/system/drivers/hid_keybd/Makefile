SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_KEYBD_OBJS = hid_keybd.o

HID_KEYBD = $(TARGET_DIR)/drivers/raspix/hid_keybd

$(HID_KEYBD): $(HID_KEYBD_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(HID_KEYBD_OBJS) -o $(HID_KEYBD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_KEYBD_OBJS)
