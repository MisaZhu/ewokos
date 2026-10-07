SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

# hid/hid_defs.h + hid/hid_joystick.h ship with the hid lib; use the source
# path too because this driver may build before the lib installs its headers
CFLAGS += -I $(BUILD_DIR)/include \
          -I $(SYS_DIR)/gui/libs/hid/include

HID_JOYSTICKD_OBJS = $(ARCH)/hid_joystickd.o

HID_JOYSTICKD = $(TARGET_DIR)/drivers/$(HW)/hid_joystickd

$(HID_JOYSTICKD): $(HID_JOYSTICKD_OBJS) \
                $(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(LD) -Ttext=100 $(HID_JOYSTICKD_OBJS) -o $(HID_JOYSTICKD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_JOYSTICKD_OBJS)
