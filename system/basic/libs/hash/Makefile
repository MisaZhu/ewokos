include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_HASH_OBJS = src/hashmap.o

HASH = $(TARGET_DIR)/lib/libhash.a

$(HASH): $(LIB_HASH_OBJS)
	$(AR) rT $(HASH) $(LIB_HASH_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_HASH_OBJS)
	rm -f $(HASH)

