MKDIR_OBJS = $(ROOT_DIR)/bin/mkdir/mkdir.o

MKDIR = $(TARGET_DIR)/$(ROOT_DIR)/bin/mkdir

PROGS += $(MKDIR)
CLEAN += $(MKDIR_OBJS)

$(MKDIR): $(MKDIR_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(MKDIR_OBJS) $(LIB_OBJS) -o $(MKDIR) $(LDFLAGS)
