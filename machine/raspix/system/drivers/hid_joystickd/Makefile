SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_JOYSTICKD_OBJS = hid_joystickd.o

HID_JOYSTICKD = $(TARGET_DIR)/drivers/raspix/hid_joystickd

$(HID_JOYSTICKD): $(HID_JOYSTICKD_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(HID_JOYSTICKD_OBJS) -o $(HID_JOYSTICKD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_JOYSTICKD_OBJS)
