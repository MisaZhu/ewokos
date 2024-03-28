include ../../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../../build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib -Llib 

CFLAGS += -I $(BUILD_DIR)/include -Iinclude

USBD_OBJS = clockwork_usbd.o

USBD = $(TARGET_DIR)/drivers/raspix/clockwork_usbd

$(USBD): $(USBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		lib/libcsud.a
	$(LD) -Ttext=100 $(USBD_OBJS) -o $(USBD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC) -lcsud -lewoksys

clean:
	rm -f $(USBD_OBJS) $(USBD)
