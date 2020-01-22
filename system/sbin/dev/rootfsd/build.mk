ROOTFSD_OBJS = $(ROOT_DIR)/sbin/dev/rootfsd/rootfsd.o
ROOTFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/rootfsd

PROGS += $(ROOTFSD)
CLEAN += $(ROOTFSD_OBJS)

$(ROOTFSD): $(ROOTFSD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(ROOTFSD_OBJS) $(LIB_OBJS) -o $(ROOTFSD) $(LDFLAGS)
