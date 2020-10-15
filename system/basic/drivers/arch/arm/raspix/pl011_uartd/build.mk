RASPIX_PL011_UARTD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspix/pl011_uartd/pl011_uartd.o

RASPIX_PL011_UARTD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspix/pl011_uartd

PROGS += $(RASPIX_PL011_UARTD)
CLEAN += $(RASPIX_PL011_UARTD_OBJS)

$(RASPIX_PL011_UARTD): $(RASPIX_PL011_UARTD_OBJS) \
	$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(RASPIX_PL011_UARTD_OBJS) -o $(RASPIX_PL011_UARTD) $(LDFLAGS) -larch_bcm283x -lewokc -lc
