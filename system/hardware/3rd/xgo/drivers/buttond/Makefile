include ../../make.inc

XGO_BUTTOND_OBJS = xgo_buttond.o

XGO_BUTTOND = $(TARGET_DIR)/drivers/$(MF)/xgo_buttond

$(XGO_BUTTOND): $(XGO_BUTTOND_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(XGO_BUTTOND_OBJS) -o $(XGO_BUTTOND) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(XGO_BUTTOND_OBJS)
