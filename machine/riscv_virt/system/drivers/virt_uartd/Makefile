SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

VIRT_UARTD_OBJS = virt_uartd.o

VIRT_UARTD = $(TARGET_DIR)/drivers/virt/virt_uartd

$(VIRT_UARTD): $(VIRT_UARTD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(VIRT_UARTD_OBJS) -o $(VIRT_UARTD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(VIRT_UARTD_OBJS) $(VIRT_UARTD)
