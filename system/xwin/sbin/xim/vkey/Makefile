SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XIM_OBJS = xim.o

XIM = $(TARGET_DIR)/sbin/x/xim_vkey

$(XIM): $(XIM_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XIM_OBJS) -o $(XIM) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

clean:
	rm -f $(XIM_OBJS)

