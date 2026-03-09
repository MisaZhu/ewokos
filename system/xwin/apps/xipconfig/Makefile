SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XIPCONFIG_OBJS = xipconfig.o

XIPCONFIG_DIR = $(TARGET_DIR)/apps/xipconfig
XIPCONFIG = $(XIPCONFIG_DIR)/xipconfig

all: $(XIPCONFIG_DIR) $(XIPCONFIG)

$(XIPCONFIG): $(XIPCONFIG_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XIPCONFIG_OBJS) -o $(XIPCONFIG) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XIPCONFIG_DIR):
	mkdir -p $(XIPCONFIG_DIR)
	cp -r res $(XIPCONFIG_DIR)

clean:
	rm -f $(XIPCONFIG_OBJS)
	rm -fr $(XIPCONFIG_DIR)

