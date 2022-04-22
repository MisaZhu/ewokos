include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

KILL_OBJS = kill.o

KILL = $(TARGET_DIR)/bin/kill

$(KILL): $(KILL_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(KILL_OBJS) -o $(KILL) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(KILL_OBJS)

