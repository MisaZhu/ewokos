SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XLOG_OBJS = xlog.o

XLOG_DIR = $(TARGET_DIR)/apps/xlog
XLOG = $(XLOG_DIR)/xlog

all: $(XLOG_DIR) $(XLOG)

$(XLOG): $(XLOG_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libtextgrid.a \
		$(BUILD_DIR)/lib/libgterminal.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XLOG_OBJS) -o $(XLOG) $(LDFLAGS)  $(EWOK_LIB_X) -lgterminal -ltextgrid $(EWOK_LIB_GRAPH)  $(EWOK_LIBC) -lcxx

$(XLOG_DIR):
	mkdir -p $(XLOG_DIR)
	cp -r res $(XLOG_DIR)

clean:
	rm -f $(XLOG_OBJS)
	rm -fr $(XLOG_DIR)

