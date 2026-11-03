SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

GREP_OBJS = $(ARCH)/grep.o
LD_LIB_DEPS = \
        $(BUILD_DIR)/lib/libewoksys.a \
        $(BUILD_DIR)/lib/libc.a \
        $(BUILD_DIR)/lib/libgloss.a

GREP = $(TARGET_DIR)/bin/grep

all: $(GREP)

$(GREP): $(GREP_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(GREP_OBJS) -o $(GREP) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(GREP_OBJS)

