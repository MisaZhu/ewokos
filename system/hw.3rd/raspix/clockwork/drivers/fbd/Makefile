include ../../make.inc

CLOCKWORK_FBD_OBJS = fbd.o

FBD = $(TARGET_DIR)/drivers/$(MF)/fbd

$(FBD): $(CLOCKWORK_FBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(CLOCKWORK_FBD_OBJS) -o $(FBD) $(LDFLAGS) -larch_bcm283x -lfbd -lupng -lgraph -lbsp -lsconf $(EWOK_LIBC)

clean:
	rm -f $(CLOCKWORK_FBD_OBJS) $(FBD)
