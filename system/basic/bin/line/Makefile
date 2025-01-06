include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

line_OBJS = line.o

line = $(TARGET_DIR)/bin/line

$(line): $(line_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(line_OBJS) -o $(line) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(line_OBJS)

