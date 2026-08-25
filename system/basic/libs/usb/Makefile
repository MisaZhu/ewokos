SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_USB_OBJS = $(ARCH)/src/usbhid.o $(ARCH)/src/usbhidsrv.o

USB = $(TARGET_DIR)/lib/libusb.a

$(USB): $(LIB_USB_OBJS)
	$(AR) rcs $(USB) $(LIB_USB_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_USB_OBJS)
	rm -f $(USB)
