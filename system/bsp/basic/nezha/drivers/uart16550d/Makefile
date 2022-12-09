include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

VIRT_UARTD_OBJS = uart16550d.o

VIRT_UARTD = $(TARGET_DIR)/drivers/nezha/uart16550d

$(VIRT_UARTD): $(VIRT_UARTD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(VIRT_UARTD_OBJS) -o $(VIRT_UARTD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(VIRT_UARTD_OBJS) $(VIRT_UARTD)
