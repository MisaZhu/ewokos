SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XCONSOLE_OBJS = xconsole.o

XCONSOLE_DIR = $(TARGET_DIR)/apps/xconsole
XCONSOLE = $(XCONSOLE_DIR)/xconsole

all: $(XCONSOLE_DIR) $(XCONSOLE)

$(XCONSOLE): $(XCONSOLE_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libtextgrid.a \
		$(BUILD_DIR)/lib/libgterminal.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XCONSOLE_OBJS) -o $(XCONSOLE) $(LDFLAGS)  -lgterminal -ltextgrid $(EWOK_LIB_X) $(EWOK_LIB_GRAPH)  $(EWOK_LIBC) -lcxx

$(XCONSOLE_DIR):
	mkdir -p $(XCONSOLE_DIR)
	cp -r res $(XCONSOLE_DIR)

clean:
	rm -f $(XCONSOLE_OBJS)
	rm -fr $(XCONSOLE_DIR)

