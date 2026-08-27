SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps
PREFIX := /opt/ewokos

ASFLAGS = --warn 
CFLAGS += -isystem $(TARGET_DIR)/include -DHAVE_CONFIG_H -I. -O2 -ffreestanding ${ARCH_CFLAGS} 
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBPNG = $(TARGET_DIR)/lib/libpng.a
all: $(LIBPNG)

PNG_ARM_SRCS := $(wildcard arm/arm_init.c arm/filter_neon.c arm/filter_neon_intrinsics.c arm/filter_neon.S)
PNG_ARM_OBJS := $(patsubst %.c,$(ARCH)/%.o,$(filter %.c,$(PNG_ARM_SRCS))) \
	$(patsubst %.S,$(ARCH)/%.o,$(filter %.S,$(PNG_ARM_SRCS)))

OBJS = $(ARCH)/png.o $(ARCH)/pngerror.o $(ARCH)/pngget.o $(ARCH)/pngmem.o $(ARCH)/pngpread.o $(ARCH)/pngread.o $(ARCH)/pngrio.o $(ARCH)/pngrtran.o $(ARCH)/pngrutil.o $(ARCH)/pngset.o $(ARCH)/pngtrans.o $(ARCH)/pngwio.o \
       $(ARCH)/pngwrite.o $(ARCH)/pngwtran.o $(ARCH)/pngwutil.o $(PNG_ARM_OBJS)

-include $(DEPDIR)/*.Po

$(LIBPNG): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r png*.h $(TARGET_DIR)/include

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@) $(DEPDIR)/$(dir $*)
	$(CC) $(CFLAGS) -std=c99 -MMD -MP -MT $@ -MF $(DEPDIR)/$*.Po -c -o $@ $<

$(ARCH)/%.o: %.cpp
	@mkdir -p $(dir $@) $(DEPDIR)/$(dir $*)
	$(CXX) $(CPPFLAGS) -MMD -MP -MT $@ -MF $(DEPDIR)/$*.Po -c -o $@ $<

$(ARCH)/%.o: %.S
	@mkdir -p $(dir $@) $(DEPDIR)/$(dir $*)
	$(CC) $(CFLAGS) -std=c99 -MMD -MP -MT $@ -MF $(DEPDIR)/$*.Po -c -o $@ $<

clean:
	rm -rf $(ARCH) libpng.a
	rm -rf $(DEPDIR)
