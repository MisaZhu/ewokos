include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XIM_OBJS = xim.o

XIM = $(TARGET_DIR)/sbin/x/xim_none

$(XIM): $(XIM_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XIM_OBJS) -o $(XIM) $(LDFLAGS) -lewoksys -lc -lgloss -lcxx

clean:
	rm -f $(XIM_OBJS)

