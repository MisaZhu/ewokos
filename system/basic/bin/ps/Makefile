SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ABI_HEADERS = $(BUILD_DIR)/include/sysinfo.h $(BUILD_DIR)/include/procinfo.h

PS_OBJS = ps.o

PS = $(TARGET_DIR)/bin/ps

all: $(PS)

ps.o: ps.c $(ABI_HEADERS)

$(PS): $(PS_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(PS_OBJS) -o $(PS) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(PS_OBJS)
