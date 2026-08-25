SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XMOUSED_OBJS = $(ARCH)/xtouch.o 

XMOUSED = $(TARGET_DIR)/sbin/x/xtouch

$(XMOUSED): $(XMOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libbsp.a
	$(LD) -Ttext=100 $(XMOUSED_OBJS) -o $(XMOUSED) $(LDFLAGS) -lx -ltinyjson -lgraph  $(BSP_LFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(XMOUSED_OBJS)
