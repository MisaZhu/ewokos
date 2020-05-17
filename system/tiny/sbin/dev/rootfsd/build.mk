ROOTFSD_OBJS = $(ROOT_DIR)/sbin/dev/rootfsd/rootfsd.o
ROOTFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/rootfsd

CLEAN += $(ROOTFSD_OBJS)  \
		$(ROOT_DIR)/sbin/dev/rootfsd/fs_data.c \
		$(ROOT_DIR)/sbin/dev/rootfsd/fs_data.o

$(ROOTFSD): $(ROOTFSD_OBJS)
	$(CC) -c $(ROOT_DIR)/sbin/dev/rootfsd/fs_data.c -o $(ROOT_DIR)/sbin/dev/rootfsd/fs_data.o $(CFLAGS)
	$(LD) -Ttext=100 $(ROOTFSD_OBJS) \
		$(ROOT_DIR)/sbin/dev/rootfsd/fs_data.o \
		-o $(ROOTFSD) $(LDFLAGS) -lewokc -lc
