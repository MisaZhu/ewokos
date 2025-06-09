SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include -I./

XSERVERD_OBJS = xserverd.o \
	cursor.o \
	xevtpool.o \
	xcmd.o \
	xtheme.o

XSERVERD = $(TARGET_DIR)/drivers/xserverd

$(XSERVERD): $(XSERVERD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libfb.a  \
		$(BUILD_DIR)/lib/libgraph.a  \
		$(BUILD_DIR)/lib/libtinyjson.a  \
		$(BUILD_DIR)/lib/libx.a  \
		$(BUILD_DIR)/lib/libfont.a  \
		$(BUILD_DIR)/lib/libdisplay.a
	$(LD) -Ttext=100 $(XSERVERD_OBJS) -o $(XSERVERD) $(LDFLAGS) -lx -lfb -ldisplay -ltinyjson $(EWOK_LIB_GRAPH) $(EWOK_LIBC)

clean:
	rm -f $(XSERVERD_OBJS)
