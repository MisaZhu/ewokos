TIMERD_OBJS = $(ROOT_DIR)/sbin/dev/timerd/timerd.o

TIMERD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/dev/timerd

PROGS += $(TIMERD)
CLEAN += $(TIMERD_OBJS)

$(TIMERD): $(TIMERD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(TIMERD_OBJS) -o $(TIMERD) $(LDFLAGS) -lewokc -lc
