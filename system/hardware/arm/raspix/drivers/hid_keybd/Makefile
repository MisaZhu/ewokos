include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_KEYBD_OBJS = hid_keybd.o

HID_KEYBD = $(TARGET_DIR)/drivers/raspix/hid_keybd

$(HID_KEYBD): $(HID_KEYBD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(HID_KEYBD_OBJS) -o $(HID_KEYBD) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(HID_KEYBD_OBJS)
