
TOOLCHAIN := arm-none-eabi-
 
CC := $(TOOLCHAIN)gcc
CXX := $(TOOLCHAIN)g++
LD := $(TOOLCHAIN)ld
AS := $(TOOLCHAIN)as
AR := $(TOOLCHAIN)ar
OBJCOPY := $(TOOLCHAIN)objcopy

DEPDIR := .deps
DEFS = -D_REENTRANT -D__EWOKOS__ -DLOAD_BMP -DLOAD_GIF -DLOAD_JPG -DLOAD_PNG -DLOAD_TGA -DHAVE_HIDDEN -DSDL_BYTEORDER=SDL_LIL_ENDIAN

PREFIX := /opt/ewokos

SDL2_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SDL2_ROOT_DIR)/../..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SDL2_ROOT_DIR)/make.inc

#ASFLAGS = --warn 
#CFLAGS = -O3 -I../SDL2/include -I../libpng -ffreestanding  $(DEFS)
ASFLAGS = --warn
CFLAGS += -O3 -I../SDL2/include -I../libpng -ffreestanding $(DEFS)
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

all: deps libSDL2_image.a

.PHONY: deps

OBJS = IMG.o IMG_bmp.o IMG_gif.o IMG_jpg.o IMG_png.o IMG_tga.o IMG_tif.o IMG_webp.o

-include $(DEPDIR)/*.Po

libSDL2_image.a: $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

deps:
	-@mkdir -p $(DEPDIR)

clean:
	rm -f *.o libSDL2_image.a
	rm -rf $(DEPDIR)
