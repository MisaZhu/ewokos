include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

STATED_OBJS = stated.o

STATED = $(TARGET_DIR)/drivers/proc/stated

$(STATED): $(STATED_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(STATED_OBJS) -o $(STATED) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(STATED_OBJS)
