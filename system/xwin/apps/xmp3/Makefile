SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XMP3_OBJS = xmp3.o

XMP3_DIR = $(TARGET_DIR)/apps/xmp3
XMP3 = $(XMP3_DIR)/xmp3

all: $(XMP3_DIR) $(XMP3)

$(XMP3): $(XMP3_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libminimp3.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a
	$(LD) -Ttext=100 $(XMP3_OBJS) -o $(XMP3) $(LDFLAGS)  $(EWOK_LIB_X) -lminimp3 $(EWOK_LIB_GRAPH)  $(EWOK_LIBC) -lcxx

$(XMP3_DIR):
	mkdir -p $(XMP3_DIR)
	cp -r res $(XMP3_DIR)

clean:
	rm -f $(XMP3_OBJS)
	rm -fr $(XMP3_DIR)