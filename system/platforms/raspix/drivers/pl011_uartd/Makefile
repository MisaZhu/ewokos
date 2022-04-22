include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

PL011_UARTD_OBJS = pl011_uartd.o

PL011_UARTD = $(TARGET_DIR)/drivers/raspix/pl011_uartd

$(PL011_UARTD): $(PL011_UARTD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(PL011_UARTD_OBJS) -o $(PL011_UARTD) $(LDFLAGS) -larch_bcm283x -lewokc -lc

clean:
	rm -f $(PL011_UARTD_OBJS) $(PL011_UARTD)
