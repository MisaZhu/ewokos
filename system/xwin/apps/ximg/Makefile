SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XIMG_OBJS = ximg.o

XIMG_DIR = $(TARGET_DIR)/apps/ximg
XIMG = $(XIMG_DIR)/ximg

all: $(XIMG_DIR) $(XIMG)

$(XIMG): $(XIMG_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libcxx.a \
		$(BUILD_DIR)/lib/libjpeg.a \
		$(BUILD_DIR)/lib/libgif.a
	$(LD) -Ttext=100 $(XIMG_OBJS) -o $(XIMG) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -ljpeg -lgif $(EWOK_LIBC) -lcxx

$(XIMG_DIR):
	mkdir -p $(XIMG_DIR)
	cp -r res $(XIMG_DIR)

clean:
	rm -f $(XIMG_OBJS)
	rm -fr $(XIMG_DIR)
