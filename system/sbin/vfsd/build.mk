VFSD_OBJS = $(ROOT_DIR)/sbin/vfsd/vfsd.o

VFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/vfsd

PROGS += $(VFSD)
CLEAN += $(VFSD_OBJS)

$(VFSD): $(VFSD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(VFSD_OBJS) -o $(VFSD) $(LDFLAGS) -lewokc -lc
