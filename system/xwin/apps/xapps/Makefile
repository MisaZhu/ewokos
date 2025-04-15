SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XAPPS_OBJS = xapps.o

XAPPS_DIR = $(TARGET_DIR)/apps/xapps
XAPPS = $(XAPPS_DIR)/xapps

all: $(XAPPS_DIR) $(XAPPS)

$(XAPPS): $(XAPPS_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XAPPS_OBJS) -o $(XAPPS) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lelf $(EWOK_LIBC) -lcxx

$(XAPPS_DIR):
	mkdir -p $(XAPPS_DIR)
	cp -r res $(XAPPS_DIR)

clean:
	rm -f $(XAPPS_OBJS)
	rm -fr $(XAPPS_DIR)

