include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include -I./

XSERVERD_OBJS = xserverd.o  cursor.o xevtpool.o

XSERVERD = $(TARGET_DIR)/drivers/xserverd

$(XSERVERD): $(XSERVERD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libsconf.a  \
		$(BUILD_DIR)/lib/libfb.a  \
		$(BUILD_DIR)/lib/libgraph.a  \
		$(BUILD_DIR)/lib/libdisplay.a
	$(LD) -Ttext=100 $(XSERVERD_OBJS) -o $(XSERVERD) $(LDFLAGS) -lgraph -lfb -ldisplay -lsconf -lewokc -lc

clean:
	rm -f $(XSERVERD_OBJS)
