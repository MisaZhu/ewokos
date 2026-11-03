SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

HEAD_OBJS = $(ARCH)/head.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

HEAD = $(TARGET_DIR)/bin/head

all: $(HEAD)

$(HEAD): $(HEAD_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(HEAD_OBJS) -o $(HEAD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(HEAD_OBJS)
