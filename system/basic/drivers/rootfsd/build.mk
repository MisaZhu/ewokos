ROOTFSD_OBJS = $(ROOT_DIR)/drivers/rootfsd/rootfsd.o
ROOTFSD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/rootfsd

PROGS += $(ROOTFSD)
CLEAN += $(ROOTFSD_OBJS)

$(ROOTFSD): $(ROOTFSD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(ROOTFSD_OBJS) -o $(ROOTFSD) $(LDFLAGS) -lext2 -lewokc -lc
