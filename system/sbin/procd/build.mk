PROCD_OBJS = $(ROOT_DIR)/sbin/procd/procd.o

PROCD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/procd

PROGS += $(PROCD)
CLEAN += $(PROCD_OBJS)

$(PROCD): $(PROCD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(PROCD_OBJS) -o $(PROCD) $(LDFLAGS) -lhash -lewokc -lc 
