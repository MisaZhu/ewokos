ROOTMEMFSD_OBJS = $(ROOT_DIR)/drivers/rootmemfsd/rootmemfsd.o
ROOTMEMFSD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/rootmemfsd

PROGS += $(ROOTMEMFSD)
CLEAN += $(ROOTMEMFSD_OBJS)

$(ROOTMEMFSD): $(ROOTMEMFSD_OBJS)
	$(LD) -Ttext=100 $(ROOTMEMFSD_OBJS) -o $(ROOTMEMFSD) $(LDFLAGS) -lewokc -lc
