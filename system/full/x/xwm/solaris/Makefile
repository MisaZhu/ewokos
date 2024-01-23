include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I. -I $(BUILD_DIR)/include

XWM_OBJS = xwm.o SolarisWM.o

XWM = $(TARGET_DIR)/sbin/x/xwm_solaris

$(XWM): $(XWM_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XWM_OBJS) -o $(XWM) $(LDFLAGS) -lx++ -lx -lgraph -lbsp -lttf -lfont  -lupng -lsconf -lewoksys -lc -lgloss -lcxx

clean:
	rm -f $(XWM_OBJS)
