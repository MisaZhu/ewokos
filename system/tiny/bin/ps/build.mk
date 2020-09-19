PS_OBJS = $(ROOT_DIR)/bin/ps/ps.o

PS = $(TARGET_DIR)/$(ROOT_DIR)/bin/ps

PROGS += $(PS)
CLEAN += $(PS_OBJS)

$(PS): $(PS_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(PS_OBJS) -o $(PS) $(LDFLAGS) -lewokc -lc
