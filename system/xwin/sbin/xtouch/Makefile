SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XMOUSED_OBJS = xtouch.o 

XMOUSED = $(TARGET_DIR)/sbin/x/xtouch

$(XMOUSED): $(XMOUSED_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libx.a
	$(LD) -Ttext=100 $(XMOUSED_OBJS) -o $(XMOUSED) $(LDFLAGS) -lx -ltinyjson -lgraph -lbsp $(BSP_LFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(XMOUSED_OBJS)
