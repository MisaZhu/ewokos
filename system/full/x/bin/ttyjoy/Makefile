include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TTYJOY_OBJS = ttyjoy.o

TTYJOY = $(TARGET_DIR)/bin/ttyjoy

$(TTYJOY): $(TTYJOY_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(TTYJOY_OBJS) -o $(TTYJOY) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(TTYJOY_OBJS)

