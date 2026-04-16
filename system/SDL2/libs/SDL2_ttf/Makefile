SDL2_ROOT_DIR=../..
EWOKOS_ROOT_DIR = $(SDL2_ROOT_DIR)/../..
SYS_ROOT_DIR = $(EWOKOS_ROOT_DIR)/system
include $(SDL2_ROOT_DIR)/make.inc

#ASFLAGS = --warn 
#CFLAGS = -O3 -I. -I../SDL2/include -I../libogg/include -I../libvorbis/include -ffreestanding  -D_REENTRANT -D__EWOKOS__ -DOGG_MUSIC=1 -DWAV_MUSIC=1
ASFLAGS = --warn
CFLAGS += -O3 -I. -I../SDL2/include -ffreestanding -D_REENTRANT -D__EWOKOS__ -DOGG_MUSIC=1 -DWAV_MUSIC=1
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti
CFLAGS += -D_REENTRANT -Wall -fvisibility=hidden -DPACKAGE_VERSION=\"2.24.0\" -DPACKAGE_STRING=\"SDL2_ttf\ 2.24.0\" -DPACKAGE_BUGREPORT=\"https://github.com/libsdl-org/SDL_ttf/issues\" -DPACKAGE_URL=\"\" -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -DHAVE_DLFCN_H=1 -DLT_OBJDIR=\".libs/\" -DSDL_BUILD_MAJOR_VERSION=2 -DSDL_BUILD_MINOR_VERSION=24 -DSDL_BUILD_MICRO_VERSION=0 -DPACKAGE=\"SDL2_ttf\" -DVERSION=\"2.24.0\" -DTTF_USE_HARFBUZZ=0 -DX_DISPLAY_MISSING=1  -DFT2_BUILD_LIBRARY -DFT_PUBLIC_FUNCTION_ATTRIBUTE=  

all: libSDL2_ttf.a

OBJS = SDL_ttf.o 

libSDL2_ttf.a: $(OBJS) Makefile
	$(AR) rcs -o libSDL2_ttf.a $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -std=c99 -c -o $@ $<


%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) libSDL2_ttf.a

