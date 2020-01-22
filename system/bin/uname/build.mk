UNAME_OBJS = $(ROOT_DIR)/bin/uname/uname.o

UNAME = $(TARGET_DIR)/$(ROOT_DIR)/bin/uname

PROGS += $(UNAME)
CLEAN += $(UNAME_OBJS)

$(UNAME): $(UNAME_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(UNAME_OBJS) $(LIB_OBJS) -o $(UNAME) $(LDFLAGS)
