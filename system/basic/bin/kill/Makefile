include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

KILL_OBJS = kill.o

KILL = $(TARGET_DIR)/bin/kill

$(KILL): $(KILL_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(KILL_OBJS) -o $(KILL) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(KILL_OBJS)

