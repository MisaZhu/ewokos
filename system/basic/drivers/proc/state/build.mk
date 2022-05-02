STATED_OBJS = $(ROOT_DIR)/drivers/proc/state/stated.o

STATED = $(TARGET_DIR)/$(ROOT_DIR)/drivers/proc/stated

PROGS += $(STATED)
CLEAN += $(STATED_OBJS) $(STATED)

$(STATED): $(STATED_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(STATED_OBJS) -o $(STATED) $(LDFLAGS) -lewokc -lc