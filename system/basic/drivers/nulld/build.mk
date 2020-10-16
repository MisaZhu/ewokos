NULLD_OBJS = $(ROOT_DIR)/drivers/nulld/nulld.o

NULLD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/nulld

PROGS += $(NULLD)
CLEAN += $(NULLD_OBJS) $(NULLD)

$(NULLD): $(NULLD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(NULLD_OBJS) -o $(NULLD) $(LDFLAGS) -lewokc -lc
