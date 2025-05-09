SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MD5_OBJS = md5.o

MD5 = $(TARGET_DIR)/bin/md5

$(MD5): $(MD5_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(MD5_OBJS) -o $(MD5) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MD5_OBJS)

