include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XJOYMOUSED_OBJS = xjoymoused.o 

XJOYMOUSED = $(TARGET_DIR)/sbin/x/xjoymoused

$(XJOYMOUSED): $(XJOYMOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(XJOYMOUSED_OBJS) -o $(XJOYMOUSED) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(XJOYMOUSED_OBJS)
