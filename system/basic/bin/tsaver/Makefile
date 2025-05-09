SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

TSAVER_OBJS = tsaver.o

TSAVER = $(TARGET_DIR)/bin/tsaver

$(TSAVER): $(TSAVER_OBJS) \
		$(EWOK_LIBC_A)
		$(LD) -Ttext=100 $(TSAVER_OBJS) -o $(TSAVER) $(LDFLAGS) $(EWOK_LIBC) 

clean:
	rm -f $(TSAVER_OBJS)

