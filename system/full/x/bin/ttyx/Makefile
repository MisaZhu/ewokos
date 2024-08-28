include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TTYX_OBJS = ttyx.o

TTYX = $(TARGET_DIR)/bin/ttyx

$(TTYX): $(TTYX_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(TTYX_OBJS) -o $(TTYX) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TTYX_OBJS)

