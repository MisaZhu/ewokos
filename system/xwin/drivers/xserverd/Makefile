SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include -I./

XSERVERD_OBJS = $(ARCH)/xserverd.o \
	$(ARCH)/xserver_dev.o \
	$(ARCH)/xwin.o \
	$(ARCH)/xrender.o \
	$(ARCH)/xrepaint.o \
	$(ARCH)/xinput.o \
	$(ARCH)/xwin_cmd.o \
	$(ARCH)/xshm.o \
	$(ARCH)/cursor.o \
	$(ARCH)/xevtpool.o \
	$(ARCH)/xcmd.o \
	$(ARCH)/xtheme.o

XSERVERD = $(TARGET_DIR)/drivers/xserverd

$(XSERVERD): $(XSERVERD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libdisplay.a  \
		$(BUILD_DIR)/lib/libgraph.a  \
		$(BUILD_DIR)/lib/libtinyjson.a  \
		$(BUILD_DIR)/lib/libx.a  \
		$(BUILD_DIR)/lib/libfont.a  \
		$(BUILD_DIR)/lib/libdisplayman.a
	$(LD) -Ttext=100 $(XSERVERD_OBJS) -o $(XSERVERD) $(LDFLAGS) -lx -ltinyjson $(EWOK_LIB_GRAPH) $(EWOK_LIBC)

clean:
	rm -f $(XSERVERD_OBJS)
