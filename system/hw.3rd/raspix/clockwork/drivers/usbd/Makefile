include ../../make.inc

LDFLAGS += -Llib 

CFLAGS += -I include

USBD_OBJS = usbd.o

USBD = $(TARGET_DIR)/drivers/$(MF)/usbd

$(USBD): $(USBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		lib/libcsud.a
	$(LD) -Ttext=100 $(USBD_OBJS) -o $(USBD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC) -lcsud -lewoksys

clean:
	rm -f $(USBD_OBJS) $(USBD)
