
TOOLCHAIN := arm-none-eabi-
 
CC := $(TOOLCHAIN)gcc
CXX := $(TOOLCHAIN)g++
LD := $(TOOLCHAIN)ld
AS := $(TOOLCHAIN)as
AR := $(TOOLCHAIN)ar
OBJCOPY := $(TOOLCHAIN)objcopy

DEPDIR := .deps

PREFIX := /opt/ewokos

SDL2_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SDL2_ROOT_DIR)/../..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SDL2_ROOT_DIR)/make.inc

ASFLAGS = --warn
CFLAGS += -O3 -I. -I../SDL2/include -ffreestanding -D_REENTRANT -D__EWOKOS__
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

all: deps libSDL2_gfx.a

.PHONY: deps

OBJS = SDL2_framerate.o SDL2_gfxPrimitives.o SDL2_imageFilter.c  SDL2_rotozoom.o

-include $(DEPDIR)/*.Po

libSDL2_gfx.a: $(OBJS) Makefile
	$(AR) rcs -o libSDL2_gfx.a $(OBJS)

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
	rm -f *.o libSDL2_gfx.a
	rm -rf $(DEPDIR)
