SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

UMOUNT_OBJS = $(ARCH)/umount.o

UMOUNT = $(TARGET_DIR)/bin/umount

$(UMOUNT): $(UMOUNT_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(UMOUNT_OBJS) -o $(UMOUNT) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(UMOUNT_OBJS)

