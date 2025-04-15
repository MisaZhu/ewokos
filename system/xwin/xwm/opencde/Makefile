SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I. -I $(BUILD_DIR)/include

XWM_OBJS = xwm.o OpenCDEWM.o

XWM = $(TARGET_DIR)/sbin/x/xwm_opencde

$(XWM): $(XWM_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XWM_OBJS) -o $(XWM) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

clean:
	rm -f $(XWM_OBJS)
