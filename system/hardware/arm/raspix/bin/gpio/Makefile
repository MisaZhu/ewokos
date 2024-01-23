include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

GPIO_OBJS = gpio.o

GPIO = $(TARGET_DIR)/bin/gpio

$(GPIO): $(GPIO_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(GPIO_OBJS) -o $(GPIO) $(LDFLAGS) -larch_bcm283x -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(GPIO_OBJS)

