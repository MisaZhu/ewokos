KSERVD_OBJS = $(ROOT_DIR)/sbin/kservd/kservd.o

KSERVD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/kservd

PROGS += $(KSERVD)
CLEAN += $(KSERVD_OBJS)

$(KSERVD): $(KSERVD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(KSERVD_OBJS) -o $(KSERVD) $(LDFLAGS) -lewokc -lsconf -lc
