include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_FONTS_OBJS = src/fonts.o \
	src/font10x20.o \
	src/font12x24.o \
	src/font6x8.o \
	src/font8x10.o \
	src/font8x16.o \
	src/font9x16.o \
	src/font12x16.o \
	src/font16x32.o \
	src/font5x12.o \
	src/font7x9.o \
	src/font8x8.o \
	src/font9x8.o

FONTS = $(TARGET_DIR)/lib/libfonts.a

$(FONTS): $(LIB_FONTS_OBJS)
	$(AR) rT $(FONTS) $(LIB_FONTS_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_FONTS_OBJS)
	rm -f $(FONTS)

