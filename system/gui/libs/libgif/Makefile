SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps
PREFIX := /opt/ewokos

CFLAGS += -isystem $(TARGET_DIR)/include -DHAVE_CONFIG_H -I. -O2 -ffreestanding ${ARCH_CFLAGS} -include config.h
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBGIF = $(TARGET_DIR)/lib/libgif.a
all: $(LIBGIF)

# libgif source files (giflib)
OBJS = $(ARCH)/dgif_lib.o $(ARCH)/egif_lib.o $(ARCH)/gifalloc.o $(ARCH)/gif_err.o $(ARCH)/gif_hash.o $(ARCH)/gif_font.o \
       $(ARCH)/gif_hash.o $(ARCH)/openbsd-reallocarray.o

-include $(DEPDIR)/*.Po

$(LIBGIF): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r *.h $(TARGET_DIR)/include

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@) $(DEPDIR)/$(dir $*)
	$(CC) $(CFLAGS) -std=c99 -MMD -MP -MT $@ -MF $(DEPDIR)/$*.Po -c -o $@ $<

clean:
	rm -rf $(ARCH) libgif.a
	rm -rf $(DEPDIR)
