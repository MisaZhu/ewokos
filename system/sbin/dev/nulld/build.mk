NULLD_OBJS = $(ROOT_DIR)/sbin/dev/nulld/nulld.o

NULLD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/nulld

PROGS += $(NULLD)
CLEAN += $(NULLD_OBJS)

$(NULLD): $(NULLD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(NULLD_OBJS) $(LIB_OBJS) -o $(NULLD) $(LDFLAGS)
