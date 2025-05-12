SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

UARTD_OBJS = uartd.o

UARTD = $(TARGET_DIR)/drivers/raspix/uartd

$(UARTD): $(UARTD_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(UARTD_OBJS) -o $(UARTD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(UARTD_OBJS) $(UARTD)
