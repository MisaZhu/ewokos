include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

NEZHA_UARTD_OBJS = uart16550d.o

NEZHA_UARTD = $(TARGET_DIR)/drivers/nezha/uart16550d

$(NEZHA_UARTD): $(NEZHA_UARTD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(NEZHA_UARTD_OBJS) -o $(NEZHA_UARTD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(NEZHA_UARTD_OBJS) $(NEZHA_UARTD)
