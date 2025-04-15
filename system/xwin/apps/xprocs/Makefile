SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XPROCS_OBJS = xprocs.o

XPROCS_DIR = $(TARGET_DIR)/apps/xprocs
XPROCS = $(XPROCS_DIR)/xprocs

all: $(XPROCS_DIR) $(XPROCS)

$(XPROCS): $(XPROCS_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XPROCS_OBJS) -o $(XPROCS) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XPROCS_DIR):
	mkdir -p $(XPROCS_DIR)
	cp -r res $(XPROCS_DIR)

clean:
	rm -f $(XPROCS_OBJS)
	rm -fr $(XPROCS_DIR)

