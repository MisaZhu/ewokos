CORE_OBJS = $(ROOT_DIR)/sbin/core/core.o 
	
CORE = $(TARGET_DIR)/$(ROOT_DIR)/sbin/core

PROGS += $(CORE)
CLEAN += $(CORE_OBJS) $(CORE)

$(CORE): $(CORE_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libarch_vpb.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(CORE_OBJS) -o $(CORE) $(LDFLAGS) -lsd -lext2 -larch_bcm283x -larch_vpb -lhash -lewokc -lc -lnosys
	$(OBJDUMP) -D $(CORE) > $(BUILD_DIR)/asm/core.asm
