include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

XMOUSED_OBJS = xtouchd.o 

XMOUSED = $(TARGET_DIR)/sbin/x/xtouchd

$(XMOUSED): $(XMOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libx.a
	$(LD) -Ttext=100 $(XMOUSED_OBJS) -o $(XMOUSED) $(LDFLAGS) -lx -lgraph -lewokc -lc -lsconf

clean:
	rm -f $(XMOUSED_OBJS)
