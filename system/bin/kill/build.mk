KILL_OBJS = $(ROOT_DIR)/bin/kill/kill.o

KILL = $(TARGET_DIR)/$(ROOT_DIR)/bin/kill

PROGS += $(KILL)
CLEAN += $(KILL_OBJS)

$(KILL): $(KILL_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(KILL_OBJS) $(LIB_OBJS) -o $(KILL) $(LDFLAGS)
