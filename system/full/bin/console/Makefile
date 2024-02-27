include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CONSOLE_OBJS = console.o

CONSOLE = $(TARGET_DIR)/bin/console

$(CONSOLE): $(CONSOLE_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(CONSOLE_OBJS) -o $(CONSOLE) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(CONSOLE_OBJS)

