KILL_OBJS = $(ROOT_DIR)/bin/kill/kill.o

KILL = $(TARGET_DIR)/$(ROOT_DIR)/bin/kill

PROGS += $(KILL)
CLEAN += $(KILL_OBJS)

$(KILL): $(KILL_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(KILL_OBJS) -o $(KILL) $(LDFLAGS) -lewokc -lc
