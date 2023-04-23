include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

MINI_UARTD_OBJS = mini_uartd.o

MINI_UARTD = $(TARGET_DIR)/drivers/raspix/mini_uartd

$(MINI_UARTD): $(MINI_UARTD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(MINI_UARTD_OBJS) -o $(MINI_UARTD) $(LDFLAGS) -larch_bcm283x -lewokc -lc

clean:
	rm -f $(MINI_UARTD_OBJS) $(MINI_UARTD)
