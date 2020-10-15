AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

# flags
CFLAGS = -Wstrict-prototypes \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-memset \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

#CFLAGS +=  -g
CFLAGS +=  -O2

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_FONTS_OBJS = src/fonts.o \
	src/font10x20.o \
	src/font12x24.o \
	src/font4x6.o \
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

