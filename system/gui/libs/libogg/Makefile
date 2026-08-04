SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

DEPDIR := .deps
PREFIX := /opt/ewokos

ASFLAGS = --warn
CFLAGS += -DHAVE_CONFIG_H -I. -Iinclude -I $(TARGET_DIR)/include -O2 -ffreestanding ${ARCH_CFLAGS}
CPPFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIBOGG = $(TARGET_DIR)/lib/libogg.a
all: $(LIBOGG)

OBJS = $(ARCH)/bitwise.o $(ARCH)/framing.o

-include $(DEPDIR)/*.Po

$(LIBOGG): $(OBJS) Makefile
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
	rm -rf $(ARCH) libogg.a
	rm -rf $(DEPDIR)
