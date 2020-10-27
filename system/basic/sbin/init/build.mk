INIT_OBJS = $(ROOT_DIR)/sbin/init/init.o \
		$(ROOT_DIR)/sbin/init/core.o \
		$(ROOT_DIR)/sbin/init/procd.o \
		$(ROOT_DIR)/sbin/init/vfsd.o \
		$(ROOT_DIR)/sbin/init/rootfs/romfs/romfsd.o \
		$(ROOT_DIR)/sbin/init/rootfs/sdfs/sdfsd.o
	
INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

PROGS += $(INIT)
CLEAN += $(INIT_OBJS) $(INIT)

$(INIT): $(INIT_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libarch_vpb.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(INIT_OBJS) -o $(INIT) $(LDFLAGS) -lsd -lext2 -larch_bcm283x -larch_vpb -lhash -lewokc -lc -lnosys
	$(OBJDUMP) -D $(INIT) > $(BUILD_DIR)/asm/init.asm
