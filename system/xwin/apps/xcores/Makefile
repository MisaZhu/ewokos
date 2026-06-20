SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

GCORES_OBJS = xcores.o
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

GCORES_DIR = $(TARGET_DIR)/apps/xcores
GCORES = $(GCORES_DIR)/xcores

all: $(GCORES_DIR) $(GCORES)

$(GCORES): $(GCORES_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(GCORES_OBJS) -o $(GCORES) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(GCORES_DIR):
	mkdir -p $(GCORES_DIR)
	cp -r res $(GCORES_DIR)

clean:
	rm -f $(GCORES_OBJS)
	rm -fr $(GCORES_DIR)
