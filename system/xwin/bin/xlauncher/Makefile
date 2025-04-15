SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XLAUNCHER_OBJS = xlauncher.o

XLAUNCHER_DIR = $(TARGET_DIR)/bin/x
XLAUNCHER = $(XLAUNCHER_DIR)/xlauncher

all: $(XLAUNCHER_DIR) $(XLAUNCHER)

$(XLAUNCHER): $(XLAUNCHER_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XLAUNCHER_OBJS) -o $(XLAUNCHER) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XLAUNCHER_DIR):
	mkdir -p $(XLAUNCHER_DIR)
	cp -r res $(XLAUNCHER_DIR)

clean:
	rm -f $(XLAUNCHER_OBJS)
	rm -fr $(XLAUNCHER_DIR)

