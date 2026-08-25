SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XIM_OBJS = $(ARCH)/xim.o

XIM = $(TARGET_DIR)/sbin/x/xim_none

$(XIM): $(XIM_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libcxx.a \
		$(BUILD_DIR)/lib/libdisplay.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libdisplayman.a
	$(LD) -Ttext=100 $(XIM_OBJS) -o $(XIM) $(LDFLAGS) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

clean:
	rm -f $(XIM_OBJS)

