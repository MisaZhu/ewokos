SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps

PREFIX := /opt/ewokos

ASFLAGS = --warn
CFLAGS = -O3 -ffreestanding ${ARCH_CFLAGS} -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN -I. 
CPPFLAGS = $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBZ = $(TARGET_DIR)/lib/libz.a
all: $(LIBZ)

OBJS = adler32.o crc32.o  deflate.o infback.o inffast.o inflate.o inftrees.o trees.o zutil.o \
       compress.o uncompr.o \
       gzclose.o gzlib.o gzread.o gzwrite.o

-include $(DEPDIR)/*.Po

$(LIBZ): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r *.h $(TARGET_DIR)/include

%.o: %.c
	@mkdir -p $(DEPDIR)/$(@D)
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.cpp
	@mkdir -p $(DEPDIR)/$(@D)
	$(CXX) $(CPPFLAGS) -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f *.o libz.a
	rm -rf $(DEPDIR)
