ROMFSD_OBJS = $(ROOT_DIR)/drivers/romfsd/romfsd.o
ROMFSD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/romfsd

PROGS += $(ROMFSD)
CLEAN += $(ROMFSD_OBJS)

$(ROMFSD): $(ROMFSD_OBJS)
	$(LD) -Ttext=100 $(ROMFSD_OBJS) -o $(ROMFSD) $(LDFLAGS) -lewokc -lc
