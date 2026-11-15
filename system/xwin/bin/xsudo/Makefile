SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XSUDO_OBJS = $(ARCH)/xsudo.o

XSUDO = $(TARGET_DIR)/bin/x/xsudo

$(XSUDO): $(XSUDO_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libcxx.a \
		$(BUILD_DIR)/lib/libwidget++.a
	$(LD) -Ttext=100 $(XSUDO_OBJS) -o $(XSUDO) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

clean:
	rm -f $(XSUDO_OBJS)
