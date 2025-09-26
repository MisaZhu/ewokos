SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

SPLASHD_OBJS = splashd.o

SPLASHD = $(TARGET_DIR)/sbin/splashd

$(SPLASHD): $(SPLASHD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libfb.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfont.a  \
		$(BUILD_DIR)/lib/libdisplay.a 
	$(LD) -Ttext=100 $(SPLASHD_OBJS) -o $(SPLASHD) $(LDFLAGS) $(EWOK_LIB_GRAPH) $(EWOK_LIBC)

clean:
	rm -f $(SPLASHD_OBJS) $(SPLASHD)
