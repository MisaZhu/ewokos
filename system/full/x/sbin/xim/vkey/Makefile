include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XIM_OBJS = xim.o

XIM = $(TARGET_DIR)/sbin/x/xim_vkey

$(XIM): $(XIM_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XIM_OBJS) -o $(XIM) $(LDFLAGS) -lgraph -lbsp -lttf -lfont -lx++ -lx -lcxx -lewoksys -lc -lgloss

clean:
	rm -f $(XIM_OBJS)

