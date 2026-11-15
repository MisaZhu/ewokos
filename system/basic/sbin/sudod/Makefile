ARCH ?= aarch64
HW ?= virt
SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SUDOD_OBJS = $(ARCH)/sudod.o

SUDOD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/sudod

$(SUDOD): $(SUDOD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(SUDOD_OBJS) -o $(SUDOD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -fr $(SUDOD) $(SUDOD_OBJS)
