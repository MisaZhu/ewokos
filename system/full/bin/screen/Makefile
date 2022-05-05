include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

SCREEN_OBJS = screen.o

SCREEN = $(TARGET_DIR)/bin/screen

$(SCREEN): $(SCREEN_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(SCREEN_OBJS) -o $(SCREEN) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(SCREEN_OBJS)

