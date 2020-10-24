KLOGD_OBJS = $(ROOT_DIR)/drivers/klogd/klogd.o

KLOGD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/klogd

PROGS += $(KLOGD)
CLEAN += $(KLOGD_OBJS) $(KLOGD)

$(KLOGD): $(KLOGD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(KLOGD_OBJS) -o $(KLOGD) $(LDFLAGS) -lewokc -lc
