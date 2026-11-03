SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

WC_OBJS = $(ARCH)/wc.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

WC = $(TARGET_DIR)/bin/wc

all: $(WC)

$(WC): $(WC_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(WC_OBJS) -o $(WC) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(WC_OBJS)
