include ../../make.inc

PMU_OBJS = powerd.o

PMU = $(TARGET_DIR)/drivers/$(MF)/powerd

$(PMU): $(PMU_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(PMU_OBJS) -o $(PMU) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(PMU_OBJS) $(PMU)
