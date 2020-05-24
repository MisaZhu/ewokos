INIT_OBJS = $(ROOT_DIR)/sbin/init/init.o \
		$(ROOT_DIR)/sbin/init/core.o
	
INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

CLEAN += $(INIT_OBJS) \
	$(ROOT_DIR)/sbin/init/vfsd_data.o \
	$(ROOT_DIR)/sbin/init/vfsd_data.c \
	$(ROOT_DIR)/sbin/init/rootfsd_data.o \
	$(ROOT_DIR)/sbin/init/rootfsd_data.c

$(INIT): $(INIT_OBJS) 
	$(CC) -c $(ROOT_DIR)/sbin/init/vfsd_data.c -o $(ROOT_DIR)/sbin/init/vfsd_data.o $(CFLAGS)
	$(CC) -c $(ROOT_DIR)/sbin/init/rootfsd_data.c -o $(ROOT_DIR)/sbin/init/rootfsd_data.o $(CFLAGS)
	$(LD) -Ttext=100 $(INIT_OBJS) \
		$(ROOT_DIR)/sbin/init/vfsd_data.o \
		$(ROOT_DIR)/sbin/init/rootfsd_data.o \
		-o $(INIT) $(LDFLAGS) -lewokc -lc -lhash
	#$(OBJDUMP) -D $(INIT) > $(BUILD_DIR)/asm/init.asm
