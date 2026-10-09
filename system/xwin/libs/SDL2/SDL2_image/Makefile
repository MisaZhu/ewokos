DEPDIR := .deps
DEFS = -D_REENTRANT -D__EWOKOS__ -DLOAD_BMP -DLOAD_GIF -DLOAD_JPG -DLOAD_PNG -DLOAD_TGA -DHAVE_HIDDEN -DSDL_BYTEORDER=SDL_LIL_ENDIAN

PREFIX := /opt/ewokos

SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

ASFLAGS = --warn
CFLAGS += -O3 -I../SDL2/include -isystem $(TARGET_DIR)/include -ffreestanding $(DEFS)
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIB = $(ARCH)/libSDL2_image.a

all: deps $(LIB)

.PHONY: deps

OBJS = $(ARCH)/IMG.o $(ARCH)/IMG_bmp.o $(ARCH)/IMG_gif.o $(ARCH)/IMG_jpg.o $(ARCH)/IMG_png.o $(ARCH)/IMG_tga.o $(ARCH)/IMG_tif.o $(ARCH)/IMG_webp.o

-include $(DEPDIR)/*.Po

$(LIB): $(OBJS) Makefile
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(OBJS)

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
	rm -rf $(ARCH) libSDL2_image.a
	rm -rf $(DEPDIR)
