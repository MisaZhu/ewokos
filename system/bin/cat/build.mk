CAT_OBJS = $(ROOT_DIR)/bin/cat/cat.o

CAT = $(TARGET_DIR)/$(ROOT_DIR)/bin/cat

PROGS += $(CAT)
CLEAN += $(CAT_OBJS)

$(CAT): $(CAT_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(CAT_OBJS) $(LIB_OBJS) -o $(CAT) $(LDFLAGS)

