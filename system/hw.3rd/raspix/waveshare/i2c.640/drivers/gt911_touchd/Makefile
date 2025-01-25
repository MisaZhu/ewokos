include ../../../make.inc 

GT911_OBJS = gt911.o gt911_touchd.o

GT911 = $(TARGET_DIR)/drivers/$(MF)/gt911_touchd

$(GT911): $(GT911_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(GT911_OBJS) -o $(GT911) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(GT911_OBJS)
