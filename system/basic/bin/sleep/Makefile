include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

SLEEP_OBJS = sleep.o

SLEEP = $(TARGET_DIR)/bin/sleep

$(SLEEP): $(SLEEP_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(SLEEP_OBJS) -o $(SLEEP) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(SLEEP_OBJS)

