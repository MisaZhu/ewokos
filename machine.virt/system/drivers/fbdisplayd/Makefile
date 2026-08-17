SYS_DIR=../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FBDISPLAYD_OBJS = $(ARCH)/fbdisplayd.o 

FBDISPLAYD = $(TARGET_DIR)/drivers/virt/fbdisplayd

$(FBDISPLAYD): $(FBDISPLAYD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libpng.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libdisplayd.a \
		$(BUILD_DIR)/lib/libarch_virt.a \
		$(BUILD_DIR)/lib/libdisplay.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libdisplayman.a \
		$(BUILD_DIR)/lib/libgterminal.a \
		$(BUILD_DIR)/lib/libtextgrid.a
	$(LD) -Ttext=100 $(FBDISPLAYD_OBJS) -o $(FBDISPLAYD) $(LDFLAGS) -lgterminal -ltextgrid -ldisplayd -ltinyjson $(EWOK_LIB_GRAPH) $(EWOK_LIBC)

clean:
	rm -f $(FBDISPLAYD_OBJS)
