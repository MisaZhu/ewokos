SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XTERM_OBJS = xterm.o

XTERM_DIR = $(TARGET_DIR)/apps/xterm
XTERM = $(XTERM_DIR)/xterm

all: $(XTERM_DIR) $(XTERM)

$(XTERM): $(XTERM_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libtextgrid.a \
		$(BUILD_DIR)/lib/libgterminal.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XTERM_OBJS) -o $(XTERM) $(LDFLAGS)  $(EWOK_LIB_X) -lgterminal -ltextgrid $(EWOK_LIB_GRAPH)  $(EWOK_LIBC) -lcxx

$(XTERM_DIR):
	mkdir -p $(XTERM_DIR)
	cp -r res $(XTERM_DIR)

clean:
	rm -f $(XTERM_OBJS)
	rm -fr $(XTERM_DIR)

