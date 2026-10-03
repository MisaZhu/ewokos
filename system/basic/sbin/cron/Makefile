ARCH ?= aarch64
HW ?= virt
SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CRON_OBJS = $(ARCH)/cron.o

CRON = $(TARGET_DIR)/sbin/cron

$(CRON): $(CRON_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(CRON_OBJS) -o $(CRON) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -fr $(CRON) $(CRON_OBJS)
