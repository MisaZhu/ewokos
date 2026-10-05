SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_HID_OBJS = $(ARCH)/src/hid_report.o $(ARCH)/src/hid_srv.o

HID = $(TARGET_DIR)/lib/libhid.a

$(HID): $(LIB_HID_OBJS)
	mkdir -p $(TARGET_DIR)/lib $(TARGET_DIR)/include
	$(AR) rcs $(HID) $(LIB_HID_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_HID_OBJS)
	rm -f $(HID)
