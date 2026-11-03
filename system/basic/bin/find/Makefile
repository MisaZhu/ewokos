SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FIND_OBJS = $(ARCH)/find.o
LD_LIB_DEPS = \
        $(BUILD_DIR)/lib/libewoksys.a \
        $(BUILD_DIR)/lib/libc.a \
        $(BUILD_DIR)/lib/libgloss.a

FIND = $(TARGET_DIR)/bin/find

all: $(FIND)

$(FIND): $(FIND_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(FIND_OBJS) -o $(FIND) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(FIND_OBJS)
