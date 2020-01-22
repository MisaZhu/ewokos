X_OBJS = $(ROOT_DIR)/bin/x/x.o

X = $(TARGET_DIR)/$(ROOT_DIR)/bin/x

PROGS += $(X)
CLEAN += $(X_OBJS)

$(X): $(X_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(X_OBJS) $(LIB_OBJS) -o $(X) $(LDFLAGS)
