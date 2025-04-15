SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XREAD_OBJS = xread.o

XREAD_DIR = $(TARGET_DIR)/apps/xread
XREAD = $(XREAD_DIR)/xread

all: $(XREAD_DIR) $(XREAD)

$(XREAD): $(XREAD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XREAD_OBJS) -o $(XREAD) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XREAD_DIR):
	mkdir -p $(XREAD_DIR)
	cp -r res $(XREAD_DIR)

clean:
	rm -f $(XREAD_OBJS)
	rm -fr $(XREAD_DIR)

