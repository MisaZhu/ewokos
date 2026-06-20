SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XPROCS_OBJS = xprocs.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libwidget++.a \
	$(BUILD_DIR)/lib/libx++.a \
	$(BUILD_DIR)/lib/libx.a \
	$(BUILD_DIR)/lib/libtinyjson.a \
	$(BUILD_DIR)/lib/libewokstl.a \
	$(BUILD_DIR)/lib/libtinyhttpsc.a \
	$(BUILD_DIR)/lib/libsocket.a \
	$(BUILD_DIR)/lib/libfont.a \
	$(BUILD_DIR)/lib/libfreetype.a \
	$(BUILD_DIR)/lib/libiconbuf.a \
	$(BUILD_DIR)/lib/libgraph.a \
	$(BUILD_DIR)/lib/libpng.a \
	$(BUILD_DIR)/lib/libjpeg.a \
	$(BUILD_DIR)/lib/libgif.a \
	$(BUILD_DIR)/lib/libtga.a \
	$(BUILD_DIR)/lib/libsvg.a \
	$(BUILD_DIR)/lib/libz.a \
	$(BUILD_DIR)/lib/libdisplay.a \
	$(BUILD_DIR)/lib/libfb.a \
	$(BUILD_DIR)/lib/libkeyb.a \
	$(BUILD_DIR)/lib/libmouse.a \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a \
	$(BUILD_DIR)/lib/libcxx.a

XPROCS_DIR = $(TARGET_DIR)/apps/xprocs
XPROCS = $(XPROCS_DIR)/xprocs

all: $(XPROCS_DIR) $(XPROCS)

$(XPROCS): $(XPROCS_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(XPROCS_OBJS) -o $(XPROCS) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XPROCS_DIR):
	mkdir -p $(XPROCS_DIR)
	cp -r res $(XPROCS_DIR)

clean:
	rm -f $(XPROCS_OBJS)
	rm -fr $(XPROCS_DIR)
