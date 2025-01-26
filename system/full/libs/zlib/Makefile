include ../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_Z_OBJS = src/adler32.o \
	src/crc32.o \
	src/inffast.o \
	src/inftrees.o \
	src/uncompr.o \
	src/compress.o \
	src/deflate.o \
	src/inflate.o \
	src/trees.o \
	src/zutil.o

Z = $(TARGET_DIR)/lib/libz.a

$(Z): $(LIB_Z_OBJS)
	$(AR) rcs $(Z) $(LIB_Z_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_Z_OBJS)
	rm -f $(Z)

