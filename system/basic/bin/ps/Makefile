SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PS_OBJS = ps.o
LD_LIB_DEPS = \
	$(BUILD_DIR)/lib/libewoksys.a \
	$(BUILD_DIR)/lib/libc.a \
	$(BUILD_DIR)/lib/libgloss.a

PS = $(TARGET_DIR)/bin/ps

all: $(PS)

$(PS): $(PS_OBJS) $(LD_LIB_DEPS)
	$(LD) -Ttext=100 $(PS_OBJS) -o $(PS) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(PS_OBJS)
