include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

UARTD_OBJS = uartd.o

UARTD = $(TARGET_DIR)/drivers/raspix/uartd

$(UARTD): $(UARTD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(UARTD_OBJS) -o $(UARTD) $(LDFLAGS) -larch_bcm283x -lewoksys -lc -lgloss

clean:
	rm -f $(UARTD_OBJS) $(UARTD)
