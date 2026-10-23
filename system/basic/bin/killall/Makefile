SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

KILLALL_OBJS = $(ARCH)/killall.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

KILLALL = $(TARGET_DIR)/bin/killall

all: $(KILLALL)

$(KILLALL): $(KILLALL_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(KILLALL_OBJS) -o $(KILLALL) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(KILLALL_OBJS)
