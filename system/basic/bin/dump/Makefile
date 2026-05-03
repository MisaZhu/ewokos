SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

DUMP_OBJS = dump.o

DUMP = $(TARGET_DIR)/bin/dump

$(DUMP): $(DUMP_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(DUMP_OBJS) -o $(DUMP) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(DUMP_OBJS)

