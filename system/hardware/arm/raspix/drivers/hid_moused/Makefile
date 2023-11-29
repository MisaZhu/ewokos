include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

HID_MOUSED_OBJS = hid_moused.o

HID_MOUSED = $(TARGET_DIR)/drivers/raspix/hid_moused

$(HID_MOUSED): $(HID_MOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(HID_MOUSED_OBJS) -o $(HID_MOUSED) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(HID_MOUSED_OBJS)
