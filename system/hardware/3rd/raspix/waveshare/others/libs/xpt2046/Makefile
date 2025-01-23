include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_XPT2046_OBJS = src/xpt2046.o 

XPT2046 = $(TARGET_DIR)/lib/libxpt2046.a

$(XPT2046): $(LIB_XPT2046_OBJS)
	$(AR) rT $(XPT2046) $(LIB_XPT2046_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_XPT2046_OBJS)
	rm -f $(XPT2046)


