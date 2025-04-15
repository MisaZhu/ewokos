SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XPNG_OBJS = xpng.o

XPNG_DIR = $(TARGET_DIR)/apps/xpng
XPNG = $(XPNG_DIR)/xpng

all: $(XPNG_DIR) $(XPNG)

$(XPNG): $(XPNG_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XPNG_OBJS) -o $(XPNG) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XPNG_DIR):
	mkdir -p $(XPNG_DIR)
	cp -r res $(XPNG_DIR)

clean:
	rm -f $(XPNG_OBJS)
	rm -fr $(XPNG_DIR)

