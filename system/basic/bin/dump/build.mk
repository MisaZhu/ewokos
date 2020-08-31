DUMP_OBJS = $(ROOT_DIR)/bin/dump/dump.o

DUMP = $(TARGET_DIR)/$(ROOT_DIR)/bin/dump

PROGS += $(DUMP)
CLEAN += $(DUMP_OBJS)

$(DUMP): $(DUMP_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(DUMP_OBJS) -o $(DUMP) $(LDFLAGS) -lewokc -lc
