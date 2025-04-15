SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XFONTS_OBJS = xfonts.o

XFONTS_DIR = $(TARGET_DIR)/apps/xfonts
XFONTS = $(XFONTS_DIR)/xfonts

all: $(XFONTS_DIR) $(XFONTS)

$(XFONTS): $(XFONTS_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XFONTS_OBJS) -o $(XFONTS) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XFONTS_DIR):
	mkdir -p $(XFONTS_DIR)
	cp -r res $(XFONTS_DIR)

clean:
	rm -f $(XFONTS_OBJS)
	rm -fr $(XFONTS_DIR)

