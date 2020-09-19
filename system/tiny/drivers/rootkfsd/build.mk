ROOTKFSD_OBJS = $(ROOT_DIR)/drivers/rootkfsd/rootkfsd.o
ROOTKFSD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/rootkfsd

PROGS += $(ROOTKFSD)
CLEAN += $(ROOTKFSD_OBJS)

$(ROOTKFSD): $(ROOTKFSD_OBJS)
	$(LD) -Ttext=100 $(ROOTKFSD_OBJS) -o $(ROOTKFSD) $(LDFLAGS) -lewokc -lc
