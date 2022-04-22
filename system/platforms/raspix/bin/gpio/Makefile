include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

GPIO_OBJS = gpio.o

GPIO = $(TARGET_DIR)/bin/gpio

$(GPIO): $(GPIO_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(GPIO_OBJS) -o $(GPIO) $(LDFLAGS) -larch_bcm283x -lewokc -lc

clean:
	rm -f $(GPIO_OBJS)

