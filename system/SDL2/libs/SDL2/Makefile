
TOOLCHAIN := arm-none-eabi-
 
CC := $(TOOLCHAIN)gcc
CXX := $(TOOLCHAIN)g++
LD := $(TOOLCHAIN)ld
AS := $(TOOLCHAIN)as
AR := $(TOOLCHAIN)ar
OBJCOPY := $(TOOLCHAIN)objcopy

DEPDIR := .deps

PREFIX := /opt/ewokos

SDL2_ROOT_DIR = ../..
EWOKOS_ROOT_DIR = $(SDL2_ROOT_DIR)/../..
include $(SDL2_ROOT_DIR)/make.inc

ASFLAGS = --warn
CFLAGS += -O3 -Iinclude -Isrc -I../kernel -ffreestanding -fvisibility=hidden -D_REENTRANT -D__EWOKOS__
CXXFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

all: libSDL2.a

OBJS = $(shell find . -name *.c -print | sed -r 's/\.c+/\.o/g')

-include $(shell find $(DEPDIR) -name *.Po -print)

libSDL2.a: $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	@mkdir -p $(DEPDIR)/$(@D)
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.cpp
	@mkdir -p $(DEPDIR)/$(@D)
	$(CXX) $(CXXFLAGS) -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) libSDL2.a
	rm -rf $(DEPDIR)
