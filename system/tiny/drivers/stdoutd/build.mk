STDOUTD_OBJS = $(ROOT_DIR)/drivers/stdoutd/stdoutd.o

STDOUTD = $(TARGET_DIR)/$(ROOT_DIR)/drivers/stdoutd

PROGS += $(STDOUTD)
CLEAN += $(STDOUTD_OBJS)

$(STDOUTD): $(STDOUTD_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(STDOUTD_OBJS) -o $(STDOUTD) $(LDFLAGS) -lewokc -lc
