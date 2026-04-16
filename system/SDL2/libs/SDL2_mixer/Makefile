
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

#ASFLAGS = --warn 
#CFLAGS = -O3 -I. -I../SDL2/include -I../libogg/include -I../libvorbis/include -ffreestanding  -D_REENTRANT -D__EWOKOS__ -DOGG_MUSIC=1 -DWAV_MUSIC=1
ASFLAGS = --warn
CFLAGS += -O3 -I. -I../SDL2/include -I$(SYS_ROOT_DIR)/gui/libs/minimp3/include -ffreestanding -D_REENTRANT -D__EWOKOS__ -DOGG_MUSIC=1 -DWAV_MUSIC=1 -DMP3_MUSIC=1
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

all: deps libSDL2_mixer.a

.PHONY: deps

OBJS = \
	dynamic_ogg.o \
	dynamic_mp3.o \
	effect_position.o \
	effect_stereoreverse.o \
	effects_internal.o \
	load_aiff.o \
	load_ogg.o \
	load_voc.o \
	mixer.o \
	music_ogg.o \
	music_mp3.o \
	music.o \
	wavestream.o

-include $(DEPDIR)/*.Po

libSDL2_mixer.a: $(OBJS) Makefile
	$(AR) rcs -o libSDL2_mixer.a $(OBJS)

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
	rm -f *.o libSDL2_mixer.a
	rm -rf $(DEPDIR)
