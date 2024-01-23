include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

VIRT_UARTD_OBJS = virt_uartd.o

VIRT_UARTD = $(TARGET_DIR)/drivers/virt/virt_uartd

$(VIRT_UARTD): $(VIRT_UARTD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(VIRT_UARTD_OBJS) -o $(VIRT_UARTD) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(VIRT_UARTD_OBJS) $(VIRT_UARTD)
