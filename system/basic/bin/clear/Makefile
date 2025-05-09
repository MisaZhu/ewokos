SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CLEAR_OBJS = clear.o

CLEAR = $(TARGET_DIR)/bin/clear

$(CLEAR): $(CLEAR_OBJS) \
		$(EWOK_LIBC_A)
		$(LD) -Ttext=100 $(CLEAR_OBJS) -o $(CLEAR) $(LDFLAGS) $(EWOK_LIBC) 

clean:
	rm -f $(CLEAR_OBJS)

