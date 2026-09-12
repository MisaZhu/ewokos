SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_USB_OBJS =

USB_HEADERS = $(wildcard include/usb/*.h)

USB = $(TARGET_DIR)/lib/libusb.a

# libusb is header-only now: the bus-independent HID implementation moved to
# libhid (system/basic/libs/hid). The empty archive is still produced so that
# existing -lusb link lines and make dependencies keep working.
$(USB): $(USB_HEADERS)
	mkdir -p $(TARGET_DIR)/lib $(TARGET_DIR)/include
	$(AR) rcs $(USB) $(LIB_USB_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_USB_OBJS)
	rm -f $(USB)
