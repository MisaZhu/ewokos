PS_OBJS = $(ROOT_DIR)/bin/ps/ps.o

PS = $(TARGET_DIR)/$(ROOT_DIR)/bin/ps

PROGS += $(PS)
CLEAN += $(PS_OBJS)

$(PS): $(PS_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(PS_OBJS) $(LIB_OBJS) -o $(PS) $(LDFLAGS)
