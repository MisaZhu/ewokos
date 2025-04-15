SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XFONT_OBJS = xfont.o

XFONT = $(TARGET_DIR)/bin/x/xfont

all: $(XFONT)

$(XFONT): $(XFONT_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XFONT_OBJS) -o $(XFONT) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XFONT_DIR):
	mkdir -p $(XFONT_DIR)
	cp -r res $(XFONT_DIR)

clean:
	rm -f $(XFONT_OBJS)
	rm -fr $(XFONT_DIR)

