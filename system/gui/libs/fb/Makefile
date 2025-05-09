SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_FB_OBJS = src/fb.o

FB = $(TARGET_DIR)/lib/libfb.a

$(FB): $(LIB_FB_OBJS)
	$(AR) rcs $(FB) $(LIB_FB_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_FB_OBJS)
	rm -f $(FB)

