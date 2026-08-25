SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

# mouse/mouse.h ships with the gui libs; use the source path because this
# driver may build before the gui libs install their headers
CFLAGS += -I $(BUILD_DIR)/include \
          -I $(SYS_DIR)/gui/libs/mouse/include

HID_MOUSED_OBJS = $(ARCH)/hid_moused.o

HID_MOUSED = $(TARGET_DIR)/drivers/$(HW)/hid_moused

$(HID_MOUSED): $(HID_MOUSED_OBJS) \
                $(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(LD) -Ttext=100 $(HID_MOUSED_OBJS) -o $(HID_MOUSED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_MOUSED_OBJS)
