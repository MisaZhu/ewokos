SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include \
          -I $(SYS_DIR)/basic/libs/usb/include

USBHOSTD_OBJS = $(ARCH)/usbhostd.o

USBHOSTD = $(TARGET_DIR)/drivers/$(HW)/usbhostd

$(USBHOSTD): $(USBHOSTD_OBJS) \
                $(BUILD_DIR)/lib/libbsp.a \
                $(BUILD_DIR)/lib/libusb.a \
                $(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(LD) -Ttext=100 $(USBHOSTD_OBJS) -o $(USBHOSTD) $(LDFLAGS) $(BSP_LFLAGS) -lusb $(EWOK_LIBC)

clean:
	rm -f $(USBHOSTD_OBJS)
