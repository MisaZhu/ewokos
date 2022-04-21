SDFSD_OBJS = $(ROOT_DIR)/sbin/rootfs/sdfs/sdfsd.o
	
SDFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/sdfsd

PROGS += $(SDFSD)
CLEAN += $(SDFSD_OBJS) $(SDFSD)

$(SDFSD): $(SDFSD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libarch_vpb.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(SDFSD_OBJS) -o $(SDFSD) $(LDFLAGS) -lsd -lext2 -larch_bcm283x -larch_vpb -lhash -lewokc -lc -lnosys
	$(OBJDUMP) -D $(SDFSD) > $(BUILD_DIR)/asm/sdfsd.asm
