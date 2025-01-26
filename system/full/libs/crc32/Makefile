include ../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_CRC32_OBJS = src/crc32.o

CRC32 = $(TARGET_DIR)/lib/libcrc32.a

$(CRC32): $(LIB_CRC32_OBJS)
	$(AR) rcs $(CRC32) $(LIB_CRC32_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_CRC32_OBJS)
	rm -f $(CRC32)

