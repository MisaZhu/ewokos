SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TOP_OBJS = $(ARCH)/top.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

TOP = $(TARGET_DIR)/bin/top

all: $(TOP)

$(TOP): $(TOP_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(TOP_OBJS) -o $(TOP) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TOP_OBJS)
