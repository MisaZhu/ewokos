SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MOUNT_OBJS = mount.o

MOUNT = $(TARGET_DIR)/bin/mount

$(MOUNT): $(MOUNT_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(MOUNT_OBJS) -o $(MOUNT) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MOUNT_OBJS)

