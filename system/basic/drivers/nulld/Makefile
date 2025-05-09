SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

NULLD_OBJS = nulld.o

NULLD = $(TARGET_DIR)/drivers/nulld

$(NULLD): $(NULLD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(NULLD_OBJS) -o $(NULLD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(NULLD_OBJS)
