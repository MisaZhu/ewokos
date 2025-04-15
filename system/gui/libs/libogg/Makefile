SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps
PREFIX := /opt/ewokos

#ASFLAGS = --warn  -mfpu=vfp -mfloat-abi=softfp
#CFLAGS += -DHAVE_CONFIG_H -I. -Iinclude -O20 -Wall -ffreestanding -marm  -mfpu=vfp -mfloat-abi=softfp -ffast-math -fsigned-char -g -O2 
ASFLAGS = --warn  -mfloat-abi=softfp
CFLAGS += -DHAVE_CONFIG_H -I. -Iinclude -O20 -Wall -ffreestanding -marm  -mfloat-abi=softfp -ffast-math -fsigned-char -g -O2 
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBOGG = $(TARGET_DIR)/lib/libogg.a
all: $(LIBOGG)

OBJS = bitwise.o framing.o

-include $(DEPDIR)/*.Po

$(LIBOGG): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp -r include/* $(TARGET_DIR)/include

%.o: %.c
	@mkdir -p $(DEPDIR)/$(@D)
	$(CC) $(CFLAGS) -std=c99 -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.cpp
	@mkdir -p $(DEPDIR)/$(@D)
	$(CXX) $(CPPFLAGS) -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

%.o: %.s
	@mkdir -p $(DEPDIR)/$(@D)
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f *.o libogg.a
	rm -rf $(DEPDIR)
