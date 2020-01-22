LS_OBJS = $(ROOT_DIR)/bin/ls/ls.o

LS = $(TARGET_DIR)/$(ROOT_DIR)/bin/ls

PROGS += $(LS)
CLEAN += $(LS_OBJS)

$(LS): $(LS_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(LS_OBJS) $(LIB_OBJS) -o $(LS) $(LDFLAGS)
