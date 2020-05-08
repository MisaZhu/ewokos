LOCKD_OBJS = $(ROOT_DIR)/sbin/lockd/lockd.o

LOCKD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/lockd

PROGS += $(LOCKD)
CLEAN += $(LOCKD_OBJS)

$(LOCKD): $(LOCKD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(LOCKD_OBJS) -o $(LOCKD) $(LDFLAGS) -lewokc -lc
