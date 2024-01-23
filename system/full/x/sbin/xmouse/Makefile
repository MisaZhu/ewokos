include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XMOUSED_OBJS = xmoused.o 

XMOUSED = $(TARGET_DIR)/sbin/x/xmoused

$(XMOUSED): $(XMOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(XMOUSED_OBJS) -o $(XMOUSED) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(XMOUSED_OBJS)
