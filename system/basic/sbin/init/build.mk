INIT_OBJS = $(ROOT_DIR)/sbin/init/init.o \
		$(ROOT_DIR)/sbin/init/core.o \
		$(ROOT_DIR)/sbin/init/procd.o \
		$(ROOT_DIR)/sbin/init/vfsd.o
	
INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

PROGS += $(INIT)
CLEAN += $(INIT_OBJS) $(INIT)

$(INIT): $(INIT_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a
	$(LD) -Ttext=100 $(INIT_OBJS) -o $(INIT) $(LDFLAGS) -lhash -lewokc -lc -lnosys
	$(OBJDUMP) -D $(INIT) > $(BUILD_DIR)/asm/init.asm
