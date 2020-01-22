DUMP_OBJS = $(ROOT_DIR)/bin/dump/dump.o

DUMP = $(TARGET_DIR)/$(ROOT_DIR)/bin/dump

PROGS += $(DUMP)
CLEAN += $(DUMP_OBJS)

$(DUMP): $(DUMP_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(DUMP_OBJS) $(LIB_OBJS) -o $(DUMP) $(LDFLAGS)
