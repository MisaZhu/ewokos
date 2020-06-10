XMOUSED_OBJS = $(ROOT_DIR)/sbin/x/xmouse/xmoused.o

XMOUSED = $(TARGET_DIR)/$(ROOT_DIR)/sbin/x/xmoused

PROGS += $(XMOUSED)
CLEAN += $(XMOUSED_OBJS)

$(XMOUSED): $(XMOUSED_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(XMOUSED_OBJS) -o $(XMOUSED) $(LDFLAGS) -lewokc -lc
