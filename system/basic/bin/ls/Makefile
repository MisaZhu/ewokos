include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

LS_OBJS = ls.o

LS = $(TARGET_DIR)/bin/ls

$(LS): $(LS_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(LS_OBJS) -o $(LS) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(LS_OBJS)

