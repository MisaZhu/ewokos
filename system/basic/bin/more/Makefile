SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

more_OBJS = $(ARCH)/more.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

more = $(TARGET_DIR)/bin/more

all: $(more)

$(more): $(more_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(more_OBJS) -o $(more) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(more_OBJS)
