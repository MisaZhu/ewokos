include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

GREP_OBJS = grep.o

GREP = $(TARGET_DIR)/bin/grep

$(GREP): $(GREP_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(GREP_OBJS) -o $(GREP) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(GREP_OBJS)

