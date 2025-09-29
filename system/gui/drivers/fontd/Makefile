SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FONTD_OBJS = fontd.o

FONTD = $(TARGET_DIR)/drivers/fontd

$(FONTD): $(FONTD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfreetype.a \
		$(BUILD_DIR)/lib/libtinyjson.a
	$(LD) -Ttext=100 $(FONTD_OBJS) -o $(FONTD) $(LDFLAGS)  -lfreetype -lgraph  $(BSP_LFLAGS) -ltinyjson $(EWOK_LIBC)

clean:
	rm -f $(FONTD_OBJS)
