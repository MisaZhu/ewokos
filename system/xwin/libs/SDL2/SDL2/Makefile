DEPDIR := .deps

PREFIX := /opt/ewokos

SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

ASFLAGS = --warn
CFLAGS += -O3 -Iinclude -Isrc -isystem $(TARGET_DIR)/include \
	-ffreestanding -fvisibility=hidden -D_REENTRANT -D__EWOKOS__
CXXFLAGS += $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti

LIB = $(ARCH)/libSDL2.a

all: $(LIB)

SRCS = $(shell find . -name '*.c' -print)
# keep only the ewokos thread backend; the generic one duplicates symbols
# and SDL picks them arbitrarily at link time (mixed sem/cond implementations)
SRCS := $(filter-out ./src/thread/generic/%,$(SRCS))
OBJS = $(patsubst %.c,$(ARCH)/%.o,$(SRCS))

-include $(shell find $(DEPDIR) -name *.Po -print)

$(LIB): $(OBJS) Makefile
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(OBJS)

$(ARCH)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(DEPDIR)/$(dir $*)
	$(CC) $(CFLAGS) -std=c99 -MMD -MP -MF $(DEPDIR)/$*.Po -c -o $@ $<

$(ARCH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(DEPDIR)/$(dir $*)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(DEPDIR)/$*.Po -c -o $@ $<

$(ARCH)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -rf $(ARCH) libSDL2.a
	rm -rf $(DEPDIR)
