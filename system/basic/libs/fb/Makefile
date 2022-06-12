include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_FB_OBJS = src/fb.o

FB = $(TARGET_DIR)/lib/libfb.a

$(FB): $(LIB_FB_OBJS)
	$(AR) rT $(FB) $(LIB_FB_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_FB_OBJS)
	rm -f $(FB)

