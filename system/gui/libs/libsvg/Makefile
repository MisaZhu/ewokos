SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I. -I$(TARGET_DIR)/include -Iplutovg -DPLUTOVG_DISABLE_FONT_FACE_CACHE_LOAD -DSTBI_NO_THREAD_LOCALS -DSTBI_NO_FAILURE_STRINGS -DSTBI_NO_JPEG -DSTBI_NO_PNG -DSTBI_NO_BMP -DSTBI_NO_PSD -DSTBI_NO_TGA -DSTBI_NO_GIF -DSTBI_NO_HDR -DSTBI_NO_PIC -DSTBI_NO_PNM -DSTBI_NO_STDIO

LIBSVG = $(TARGET_DIR)/lib/libsvg.a

# plutovg source files (core files, no font support needed for SVG rendering)
PLUTOVG_DIR = plutovg
PLUTOVG_SRCS = \
	$(PLUTOVG_DIR)/plutovg-blend.c \
	$(PLUTOVG_DIR)/plutovg-canvas.c \
	$(PLUTOVG_DIR)/plutovg-font.c \
	$(PLUTOVG_DIR)/plutovg-matrix.c \
	$(PLUTOVG_DIR)/plutovg-paint.c \
	$(PLUTOVG_DIR)/plutovg-path.c \
	$(PLUTOVG_DIR)/plutovg-rasterize.c \
	$(PLUTOVG_DIR)/plutovg-surface.c \
	$(PLUTOVG_DIR)/plutovg-ft-math.c \
	$(PLUTOVG_DIR)/plutovg-ft-raster.c \
	$(PLUTOVG_DIR)/plutovg-ft-stroker.c

PLUTOVG_OBJS = $(PLUTOVG_SRCS:.c=.o)

# libsvg source files
LIBSVG_OBJS = svg.o plutosvg.o

all: $(LIBSVG)

OBJS = $(LIBSVG_OBJS) $(PLUTOVG_OBJS)

$(LIBSVG): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp svg.h $(TARGET_DIR)/include
	cp plutosvg.h $(TARGET_DIR)/include
	cp plutovg/plutovg.h $(TARGET_DIR)/include

%.o: %.c
	$(CC) $(CFLAGS) -std=c99 -c -o $@ $<

ifeq ($(ARCH),x86)
$(PLUTOVG_DIR)/plutovg-ft-raster.o: CFLAGS += -O0 -fwrapv -fno-strict-overflow
endif

clean:
	rm -f *.o
	rm -f $(PLUTOVG_DIR)/*.o
	rm -f $(LIBSVG)
