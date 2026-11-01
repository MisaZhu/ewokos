SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

SWITCHROOT_OBJS = $(ARCH)/switchroot.o
SWITCHROOT = $(TARGET_DIR)/sbin/switchroot

$(SWITCHROOT): $(SWITCHROOT_OBJS) $(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(SWITCHROOT_OBJS) -o $@ $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -fr $(SWITCHROOT) $(SWITCHROOT_OBJS)
