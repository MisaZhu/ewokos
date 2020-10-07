STDIOD_OBJS = $(ROOT_DIR)/drivers/stdiod/stdiod.o

STDIOD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/stdiod

PROGS += $(STDIOD)
CLEAN += $(STDIOD_OBJS)

$(STDIOD): $(STDIOD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(STDIOD_OBJS) -o $(STDIOD) $(LDFLAGS) -lewokc -lc
