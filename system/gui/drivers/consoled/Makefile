SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CONSOLED_OBJS = consoled.o

CONSOLED = $(TARGET_DIR)/drivers/consoled

$(CONSOLED): $(CONSOLED_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libfb.a \
		$(BUILD_DIR)/lib/libtextgrid.a \
		$(BUILD_DIR)/lib/libgterminal.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfont.a  \
		$(BUILD_DIR)/lib/libdisplay.a 
	$(LD) -Ttext=100 $(CONSOLED_OBJS) -o $(CONSOLED) $(LDFLAGS) -lgterminal -ltextgrid -ltinyjson $(EWOK_LIB_GRAPH) $(EWOK_LIBC)

clean:
	rm -f $(CONSOLED_OBJS)
