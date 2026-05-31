SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ABI_HEADERS = $(BUILD_DIR)/include/sysinfo.h $(BUILD_DIR)/include/procinfo.h

TASK_OBJS = sysinfo.o

TASK = $(TARGET_DIR)/bin/sysinfo

all: $(TASK)

sysinfo.o: sysinfo.c $(ABI_HEADERS)

$(TASK): $(TASK_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
