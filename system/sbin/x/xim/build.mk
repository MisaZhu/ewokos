XIMD_OBJS = $(ROOT_DIR)/sbin/x/xim/ximd.o

XIMD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/x/ximd

PROGS += $(XIMD)
CLEAN += $(XIMD_OBJS)

$(XIMD): $(XIMD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(XIMD_OBJS) -o $(XIMD) $(LDFLAGS) -lewokc -lc
