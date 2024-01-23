include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HID_MOUSED_OBJS = hid_moused.o

HID_MOUSED = $(TARGET_DIR)/drivers/raspix/hid_moused

$(HID_MOUSED): $(HID_MOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(HID_MOUSED_OBJS) -o $(HID_MOUSED) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(HID_MOUSED_OBJS)
