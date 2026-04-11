SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps
PREFIX := /opt/ewokos

CFLAGS += -I $(TARGET_DIR)/include -DHAVE_CONFIG_H -I. -O2 -ffreestanding ${ARCH_CFLAGS} -include config.h
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBGIF = $(TARGET_DIR)/lib/libgif.a
all: $(LIBGIF)

# libgif source files (giflib)
OBJS = dgif_lib.o egif_lib.o gifalloc.o gif_err.o gif_hash.o gif_font.o \
       gif_hash.o openbsd-reallocarray.o

-include $(DEPDIR)/*.Po

$(LIBGIF): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r *.h $(TARGET_DIR)/include

%.o: %.c
	@mkdir -p $(DEPDIR)/$(@D)
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

clean:
	rm -f *.o libgif.a
	rm -rf $(DEPDIR)
