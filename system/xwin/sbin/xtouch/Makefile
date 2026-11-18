SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XMOUSED_OBJS = $(ARCH)/xtouch.o $(ARCH)/touch_gesture.o

XMOUSED = $(TARGET_DIR)/sbin/x/xtouch

$(XMOUSED): $(XMOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libbsp.a
	$(LD) -Ttext=100 $(XMOUSED_OBJS) -o $(XMOUSED) $(LDFLAGS) -lx -ltinyjson -lgraph -lg2dclient -lopenlibm $(BSP_LFLAGS) $(EWOK_LIBC)

$(ARCH)/xtouch.o: xtouch.c touch_gesture.h
	@mkdir -p $(ARCH)
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCH)/touch_gesture.o: touch_gesture.c touch_gesture.h
	@mkdir -p $(ARCH)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(XMOUSED_OBJS)
