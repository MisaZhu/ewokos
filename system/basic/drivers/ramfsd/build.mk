RAMFSD_OBJS = $(ROOT_DIR)/drivers/ramfsd/ramfsd.o
RAMFSD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/ramfsd

PROGS += $(RAMFSD)
CLEAN += $(RAMFSD_OBJS) $(RAMFSD)

$(RAMFSD): $(RAMFSD_OBJS)
	$(LD) -Ttext=100 $(RAMFSD_OBJS) -o $(RAMFSD) $(LDFLAGS) -lewokc -lc
