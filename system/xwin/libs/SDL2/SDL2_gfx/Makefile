DEPDIR := .deps

PREFIX := /opt/ewokos

SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

ASFLAGS = --warn
CFLAGS += -O3 -I. -I../SDL2/include -isystem $(TARGET_DIR)/include -ffreestanding -D_REENTRANT -D__EWOKOS__
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIB = $(ARCH)/libSDL2_gfx.a

all: deps $(LIB)

.PHONY: deps

OBJS = $(ARCH)/SDL2_framerate.o $(ARCH)/SDL2_gfxPrimitives.o $(ARCH)/SDL2_imageFilter.o $(ARCH)/SDL2_rotozoom.o

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
	rm -rf $(ARCH) libSDL2_gfx.a
	rm -rf $(DEPDIR)
