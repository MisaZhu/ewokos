SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MD5SUM_OBJS = $(ARCH)/md5sum.o

MD5SUM = $(TARGET_DIR)/bin/md5sum

$(MD5SUM): $(MD5SUM_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(MD5SUM_OBJS) -o $(MD5SUM) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MD5SUM_OBJS)
