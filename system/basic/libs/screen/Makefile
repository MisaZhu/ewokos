include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_SCREEN_OBJS = src/screen.o

LIB_SCREEN = $(TARGET_DIR)/lib/libscreen.a

$(LIB_SCREEN): $(LIB_SCREEN_OBJS)
	$(AR) rT $(LIB_SCREEN) $(LIB_SCREEN_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_SCREEN_OBJS)
	rm -f $(LIB_SCREEN)

