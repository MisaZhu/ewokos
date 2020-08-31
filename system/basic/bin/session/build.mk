SESSION__OBJS = $(ROOT_DIR)/bin/session/session.o

SESSION_ = $(TARGET_DIR)/$(ROOT_DIR)/bin/session

PROGS += $(SESSION_)
CLEAN += $(SESSION__OBJS)

$(SESSION_): $(SESSION__OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(SESSION__OBJS) -o $(SESSION_) $(LDFLAGS) -lewokc -lc
