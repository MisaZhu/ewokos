include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

FB_TOOL_OBJS = fbtool.o

FB_TOOL = $(TARGET_DIR)/bin/fbtool

$(FB_TOOL): $(FB_TOOL_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libgraph.a  \
		$(BUILD_DIR)/lib/libfb.a 
	$(LD) -Ttext=100 $(FB_TOOL_OBJS) -o $(FB_TOOL) $(LDFLAGS) -lfb -lgraph -lewokc -lc

clean:
	rm -f $(FB_TOOL_OBJS)

