include ../../../make.inc

GAME_KBD_OBJS = gamekbd.o

GAME_KBD = $(TARGET_DIR)/drivers/$(MF)/gamekbd

$(GAME_KBD): $(GAME_KBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(GAME_KBD_OBJS) -o $(GAME_KBD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(GAME_KBD_OBJS)
