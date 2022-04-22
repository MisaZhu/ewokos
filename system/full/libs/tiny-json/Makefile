include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_TINY_JSON_OBJS = src/tiny-json.o

TINY_JSON = $(TARGET_DIR)/lib/libtiny-json.a

$(TINY_JSON): $(LIB_TINY_JSON_OBJS)
	$(AR) rT $(TINY_JSON) $(LIB_TINY_JSON_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_TINY_JSON_OBJS)
	rm -f $(TINY_JSON)

