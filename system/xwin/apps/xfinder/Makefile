SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XFINDER_OBJS = xfinder.o

XFINDER_DIR = $(TARGET_DIR)/apps/xfinder
XFINDER = $(XFINDER_DIR)/xfinder

all: $(XFINDER_DIR) $(XFINDER)

$(XFINDER): $(XFINDER_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libelf.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XFINDER_OBJS) -o $(XFINDER) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lelf $(EWOK_LIBC) -lcxx

$(XFINDER_DIR):
	mkdir -p $(XFINDER_DIR)
	cp -r res $(XFINDER_DIR)

clean:
	rm -f $(XFINDER_OBJS)
	rm -fr $(XFINDER_DIR)

