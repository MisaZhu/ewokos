KEVENTD_OBJS = $(ROOT_DIR)/sbin/keventd/keventd.o
	
KEVENTD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/keventd

PROGS += $(KEVENTD)
CLEAN += $(KEVENTD_OBJS)

$(KEVENTD): $(KEVENTD_OBJS) 
	$(LD) -Ttext=100 $(KEVENTD_OBJS) -o $(KEVENTD) $(LDFLAGS) -lewokc -lc
