include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I. -I $(BUILD_DIR)/include

XWM_OBJS = xwm.o MacWM.o

XWM = $(TARGET_DIR)/sbin/x/xwm_macos7

$(XWM): $(XWM_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XWM_OBJS) -o $(XWM) $(LDFLAGS) -lx++ -lx -lfonts -lgraph -lupng -lewokc -lc -lcxx

clean:
	rm -f $(XWM_OBJS)
