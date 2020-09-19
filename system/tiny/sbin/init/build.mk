INIT_OBJS = $(ROOT_DIR)/sbin/init/init.o \
		$(ROOT_DIR)/sbin/init/core.o
	
INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

PROGS += $(INIT)
CLEAN += $(INIT_OBJS)


$(INIT): $(INIT_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(INIT_OBJS) -o $(INIT) $(LDFLAGS)  -lext2 -lhash -lsd  -lewokc -lc -lnosys
	$(OBJDUMP) -D $(INIT) > $(BUILD_DIR)/asm/init.asm
