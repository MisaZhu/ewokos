DEPDIR := .deps

PREFIX := /opt/ewokos

SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

ASFLAGS = --warn
CFLAGS += -O3 -I. -I../SDL2/include -I ../../minimp3/include -isystem $(TARGET_DIR)/include -ffreestanding -D_REENTRANT -D__EWOKOS__ -DOGG_MUSIC=1 -DWAV_MUSIC=1 -DMP3_MUSIC=1
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIB = $(ARCH)/libSDL2_mixer.a

all: deps $(LIB)

.PHONY: deps

OBJS = \
	$(ARCH)/dynamic_ogg.o \
	$(ARCH)/dynamic_mp3.o \
	$(ARCH)/effect_position.o \
	$(ARCH)/effect_stereoreverse.o \
	$(ARCH)/effects_internal.o \
	$(ARCH)/load_aiff.o \
	$(ARCH)/load_ogg.o \
	$(ARCH)/load_voc.o \
	$(ARCH)/mixer.o \
	$(ARCH)/music_ogg.o \
	$(ARCH)/music_mp3.o \
	$(ARCH)/music.o \
	$(ARCH)/wavestream.o

-include $(DEPDIR)/*.Po

$(LIB): $(OBJS) Makefile
	@mkdir -p $(dir $@)
	$(AR) rcs -o $@ $(OBJS)

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(DEPDIR)/$(dir $*)
	$(CC) $(CFLAGS) -std=c99 -MMD -MP -MF $(DEPDIR)/$*.Po -c -o $@ $<

$(ARCH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(DEPDIR)/$(dir $*)
	$(CXX) $(CPPFLAGS) -MMD -MP -MF $(DEPDIR)/$*.Po -c -o $@ $<

$(ARCH)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

deps:
	-@mkdir -p $(DEPDIR)

clean:
	rm -rf $(ARCH) libSDL2_mixer.a
	rm -rf $(DEPDIR)
