SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

DISPLAYD_OBJS = displayd.o

DISPLAYD = $(TARGET_DIR)/drivers/displayd

$(DISPLAYD): $(DISPLAYD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  
	$(LD) -Ttext=100 $(DISPLAYD_OBJS) -o $(DISPLAYD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(DISPLAYD_OBJS)
