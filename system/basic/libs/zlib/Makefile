SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps

PREFIX := /opt/ewokos

ASFLAGS = --warn
CFLAGS = -O3 -ffreestanding ${ARCH_CFLAGS} -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN -I. -isystem $(TARGET_DIR)/include
CPPFLAGS = $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBZ = $(TARGET_DIR)/lib/libz.a
all: $(LIBZ)

OBJS = $(ARCH)/adler32.o $(ARCH)/crc32.o  $(ARCH)/deflate.o $(ARCH)/infback.o $(ARCH)/inffast.o $(ARCH)/inflate.o $(ARCH)/inftrees.o $(ARCH)/trees.o $(ARCH)/zutil.o \
       $(ARCH)/compress.o $(ARCH)/uncompr.o \
       $(ARCH)/gzclose.o $(ARCH)/gzlib.o $(ARCH)/gzread.o $(ARCH)/gzwrite.o

-include $(DEPDIR)/*.Po

$(LIBZ): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r *.h $(TARGET_DIR)/include

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$*.Tpo)
	$(CC) $(CFLAGS) -std=c99 -MMD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

$(ARCH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)/$*.Tpo)
	$(CXX) $(CPPFLAGS) -MMD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

$(ARCH)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -rf $(ARCH) libz.a
	rm -rf $(DEPDIR)
