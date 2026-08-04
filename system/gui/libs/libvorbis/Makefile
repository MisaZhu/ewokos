SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps

ASFLAGS = --warn
CFLAGS += -DHAVE_CONFIG_H -I. -Iinclude -Ilib -I../libogg/include -I $(TARGET_DIR)/include -O2 -ffreestanding ${ARCH_CFLAGS}
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBVORBIS = $(TARGET_DIR)/lib/libvorbis.a
all: $(LIBVORBIS)

OBJS = $(ARCH)/lib/mdct.o $(ARCH)/lib/smallft.o $(ARCH)/lib/block.o $(ARCH)/lib/envelope.o $(ARCH)/lib/window.o $(ARCH)/lib/lsp.o $(ARCH)/lib/lpc.o $(ARCH)/lib/analysis.o $(ARCH)/lib/synthesis.o \
       $(ARCH)/lib/psy.o $(ARCH)/lib/info.o $(ARCH)/lib/floor1.o $(ARCH)/lib/floor0.o $(ARCH)/lib/res0.o $(ARCH)/lib/mapping0.o $(ARCH)/lib/registry.o $(ARCH)/lib/codebook.o $(ARCH)/lib/sharedbook.o \
       $(ARCH)/lib/lookup.o $(ARCH)/lib/bitrate.o $(ARCH)/lib/vorbisfile.o

-include $(DEPDIR)/*.Po

$(LIBVORBIS): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r include/* $(TARGET_DIR)/include

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$*.Tpo)
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

$(ARCH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$*.Tpo)
	$(CXX) $(CPPFLAGS) -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

$(ARCH)/%.o: %.S
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$*.Tpo)
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

clean:
	rm -f $(OBJS) libvorbis.a
	rm -rf $(DEPDIR)
