STDIND_OBJS = $(ROOT_DIR)/drivers/stdind/stdind.o

STDIND = $(TARGET_DIR)/$(ROOT_DIR)/drivers/stdind

PROGS += $(STDIND)
CLEAN += $(STDIND_OBJS)

$(STDIND): $(STDIND_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(STDIND_OBJS) -o $(STDIND) $(LDFLAGS) -lewokc -lc
