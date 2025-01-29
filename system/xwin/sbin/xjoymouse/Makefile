SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XJOYMOUSED_OBJS = xjoymoused.o 

XJOYMOUSED = $(TARGET_DIR)/sbin/x/xjoymoused

$(XJOYMOUSED): $(XJOYMOUSED_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(XJOYMOUSED_OBJS) -o $(XJOYMOUSED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(XJOYMOUSED_OBJS)
