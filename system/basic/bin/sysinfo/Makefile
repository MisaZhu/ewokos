SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = sysinfo.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

TASK = $(TARGET_DIR)/bin/sysinfo

all: $(TASK)

$(TASK): $(TASK_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
