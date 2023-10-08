include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

GREP_OBJS = grep.o

GREP = $(TARGET_DIR)/bin/grep

$(GREP): $(GREP_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(GREP_OBJS) -o $(GREP) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(GREP_OBJS)

