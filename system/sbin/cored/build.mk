CORED_OBJS = $(ROOT_DIR)/sbin/cored/cored.o

CORED = $(TARGET_DIR)/$(ROOT_DIR)/sbin/cored

PROGS += $(CORED)
CLEAN += $(CORED_OBJS)

$(CORED): $(CORED_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(CORED_OBJS) -o $(CORED) $(LDFLAGS) -lewokc -lsconf -lc
