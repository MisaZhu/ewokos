SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_MOUSED_OBJS = hid_moused.o

HID_MOUSED = $(TARGET_DIR)/drivers/raspix/hid_moused

$(HID_MOUSED): $(HID_MOUSED_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(HID_MOUSED_OBJS) -o $(HID_MOUSED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HID_MOUSED_OBJS)
