include ../../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

STATED_OBJS = stated.o

STATED = $(TARGET_DIR)/drivers/proc/stated

$(STATED): $(STATED_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(STATED_OBJS) -o $(STATED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(STATED_OBJS)
