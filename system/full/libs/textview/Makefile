include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_TEXT_VIEW_OBJS = src/textview.o

TEXT_VIEW = $(TARGET_DIR)/lib/libtextview.a

$(TEXT_VIEW): $(LIB_TEXT_VIEW_OBJS)
	$(AR) rT $(TEXT_VIEW) $(LIB_TEXT_VIEW_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_TEXT_VIEW_OBJS)
	rm -f $(TEXT_VIEW)

