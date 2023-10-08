include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

TTYJOY_OBJS = ttyjoy.o

TTYJOY = $(TARGET_DIR)/bin/ttyjoy

$(TTYJOY): $(TTYJOY_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(TTYJOY_OBJS) -o $(TTYJOY) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(TTYJOY_OBJS)

