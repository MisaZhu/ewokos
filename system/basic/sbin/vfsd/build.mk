VFSD_OBJS = $(ROOT_DIR)/sbin/vfsd/vfsd.o
	
VFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/vfsd

PROGS += $(VFSD)
CLEAN += $(VFSD_OBJS) $(VFSD)

$(VFSD): $(VFSD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libarch_vpb.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(VFSD_OBJS) -o $(VFSD) $(LDFLAGS) -lsd -lext2 -larch_bcm283x -larch_vpb -lhash -lewokc -lc -lnosys
	$(OBJDUMP) -D $(VFSD) > $(BUILD_DIR)/asm/vfsd.asm
