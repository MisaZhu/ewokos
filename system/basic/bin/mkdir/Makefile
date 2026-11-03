SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MKDIR_OBJS = $(ARCH)/mkdir.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

MKDIR = $(TARGET_DIR)/bin/mkdir

all: $(MKDIR)

$(MKDIR): $(MKDIR_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(MKDIR_OBJS) -o $(MKDIR) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MKDIR_OBJS)

