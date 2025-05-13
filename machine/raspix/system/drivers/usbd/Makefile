SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -Llib 

CFLAGS += -I $(BUILD_DIR)/include -Iinclude

USBD_OBJS = usbd.o

USBD = $(TARGET_DIR)/drivers/raspix/usbd

$(USBD): $(USBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		lib/libcsud_$(ARCH).a
	$(LD) -Ttext=100 $(USBD_OBJS) -o $(USBD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC) -lcsud_$(ARCH) $(EWOK_LIBC)

clean:
	rm -f $(USBD_OBJS)
