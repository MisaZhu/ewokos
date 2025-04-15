SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PS2KEYBD_OBJS = ps2keybd.o

PS2KEYBD = $(TARGET_DIR)/drivers/versatilepb/ps2keybd

$(PS2KEYBD): $(PS2KEYBD_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(PS2KEYBD_OBJS) -o $(PS2KEYBD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(PS2KEYBD_OBJS)
