SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FILE_OBJS = $(ARCH)/file.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

FILE = $(TARGET_DIR)/bin/file

all: $(FILE)

$(FILE): $(FILE_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(FILE_OBJS) -o $(FILE) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(FILE_OBJS)
