RASPIX_MINI_UARTD_OBJS = $(ROOT_DIR)/drivers/arch/arm/raspix/mini_uartd/mini_uartd.o

RASPIX_MINI_UARTD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/arm/raspix/mini_uartd

PROGS += $(RASPIX_MINI_UARTD)
CLEAN += $(RASPIX_MINI_UARTD_OBJS)

$(RASPIX_MINI_UARTD): $(RASPIX_MINI_UARTD_OBJS) \
	$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(RASPIX_MINI_UARTD_OBJS) -o $(RASPIX_MINI_UARTD) $(LDFLAGS) -larch_bcm283x -lewokc -lc
