INIT_OBJS = $(ROOT_DIR)/sbin/init/init.o \
		$(ROOT_DIR)/sbin/init/sdinit.o \
		$(ROOT_DIR)/sbin/init/core.o
	
INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

PROGS += $(INIT)
CLEAN += $(INIT_OBJS)

$(INIT): $(INIT_OBJS) 
	$(LD) -Ttext=100 $(INIT_OBJS) -o $(INIT) $(LDFLAGS) -lewokc -lext2 -lc -lhash
	$(OBJDUMP) -D $(INIT) > $(BUILD_DIR)/asm/init.asm
