include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MOUNT_OBJS = mount.o

MOUNT = $(TARGET_DIR)/bin/mount

$(MOUNT): $(MOUNT_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(MOUNT_OBJS) -o $(MOUNT) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(MOUNT_OBJS)

