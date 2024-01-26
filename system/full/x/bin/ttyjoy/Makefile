include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TTYJOY_OBJS = ttyjoy.o

TTYJOY = $(TARGET_DIR)/bin/ttyjoy

$(TTYJOY): $(TTYJOY_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(TTYJOY_OBJS) -o $(TTYJOY) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TTYJOY_OBJS)

