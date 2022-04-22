include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

XWM_OBJS = xwm.o 

XWM = $(TARGET_DIR)/sbin/x/xwm_macos7

$(XWM): $(XWM_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx.a  \
		$(BUILD_DIR)/lib/libsconf.a  \
		$(BUILD_DIR)/lib/libgraph.a 
	$(LD) -Ttext=100 $(XWM_OBJS) -o $(XWM) $(LDFLAGS) -lx -lgraph -lfonts -lsconf -lewokc -lc

clean:
	rm -f $(XWM_OBJS)
