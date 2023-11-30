include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

DUMP_OBJS = dump.o

DUMP = $(TARGET_DIR)/bin/dump

$(DUMP): $(DUMP_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(DUMP_OBJS) -o $(DUMP) $(LDFLAGS) -lewokc -lc_nano -lnosys

clean:
	rm -f $(DUMP_OBJS)

