SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include
FREETYPE_PATH = .

#ASFLAGS = --warn 
#CFLAGS = -O3 -I. -I../SDL2/include -I../libogg/include -I../libvorbis/include -ffreestanding  -D_REENTRANT -D__EWOKOS__ -DOGG_MUSIC=1 -DWAV_MUSIC=1
ASFLAGS = --warn
CFLAGS += -O3 -I. -I $(FREETYPE_PATH)/include -ffreestanding ${ARCH_CFLAGS} -D_REENTRANT -D__EWOKOS__
CFLAGS += -D_REENTRANT -Wall -fvisibility=hidden -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -DHAVE_DLFCN_H=1 -DLT_OBJDIR=\".libs/\" -DSDL_BUILD_MAJOR_VERSION=2 -DSDL_BUILD_MINOR_VERSION=24 -DSDL_BUILD_MICRO_VERSION=0 -DPACKAGE=\"SDL2_ttf\" -DVERSION=\"2.24.0\" -DTTF_USE_HARFBUZZ=0 -DX_DISPLAY_MISSING=1  -DFT2_BUILD_LIBRARY -DFT_PUBLIC_FUNCTION_ATTRIBUTE=  

LIB_FT = $(TARGET_DIR)/lib/libfreetype.a

all: $(LIB_FT) install_headers

OBJS = \
	$(ARCH)/$(FREETYPE_PATH)/src/autofit/autofit.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftbase.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftbbox.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftbdf.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftbitmap.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftcid.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftdebug.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftfstype.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftgasp.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftglyph.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftgxval.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftinit.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftmm.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftotval.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftpatent.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftpfr.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftstroke.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftsynth.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftsystem.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/fttype1.o \
	$(ARCH)/$(FREETYPE_PATH)/src/base/ftwinfnt.o \
	$(ARCH)/$(FREETYPE_PATH)/src/bdf/bdf.o \
	$(ARCH)/$(FREETYPE_PATH)/src/bzip2/ftbzip2.o \
	$(ARCH)/$(FREETYPE_PATH)/src/cache/ftcache.o \
	$(ARCH)/$(FREETYPE_PATH)/src/cff/cff.o \
	$(ARCH)/$(FREETYPE_PATH)/src/cid/type1cid.o \
	$(ARCH)/$(FREETYPE_PATH)/src/gzip/ftgzip.o \
	$(ARCH)/$(FREETYPE_PATH)/src/lzw/ftlzw.o \
	$(ARCH)/$(FREETYPE_PATH)/src/pcf/pcf.o \
	$(ARCH)/$(FREETYPE_PATH)/src/pfr/pfr.o \
	$(ARCH)/$(FREETYPE_PATH)/src/psaux/psaux.o \
	$(ARCH)/$(FREETYPE_PATH)/src/pshinter/pshinter.o \
	$(ARCH)/$(FREETYPE_PATH)/src/psnames/psmodule.o \
	$(ARCH)/$(FREETYPE_PATH)/src/raster/raster.o \
	$(ARCH)/$(FREETYPE_PATH)/src/sdf/sdf.o \
	$(ARCH)/$(FREETYPE_PATH)/src/sfnt/sfnt.o \
	$(ARCH)/$(FREETYPE_PATH)/src/smooth/smooth.o \
	$(ARCH)/$(FREETYPE_PATH)/src/svg/svg.o \
	$(ARCH)/$(FREETYPE_PATH)/src/truetype/truetype.o \
	$(ARCH)/$(FREETYPE_PATH)/src/type1/type1.o \
	$(ARCH)/$(FREETYPE_PATH)/src/type42/type42.o \
	$(ARCH)/$(FREETYPE_PATH)/src/winfonts/winfnt.o


$(LIB_FT): $(OBJS) Makefile
	$(AR) rcs -o $(LIB_FT) $(OBJS)

install_headers:
	rm -rf $(TARGET_DIR)/include/freetype
	cp -R include/freetype $(TARGET_DIR)/include
	cp -f include/ft2build.h $(TARGET_DIR)/include

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -std=c99 -c -o $@ $<

$(ARCH)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(LIB_FT)
