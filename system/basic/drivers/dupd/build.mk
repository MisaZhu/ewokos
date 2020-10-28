DUPD_OBJS = $(ROOT_DIR)/drivers/dupd/dupd.o

DUPD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/dupd

PROGS += $(DUPD)
CLEAN += $(DUPD_OBJS) $(DUPD)

$(DUPD): $(DUPD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(DUPD_OBJS) -o $(DUPD) $(LDFLAGS) -lewokc -lc
