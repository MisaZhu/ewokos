XWM_TINY_OBJS = $(ROOT_DIR)/sbin/x/xwm/tiny/xwm.o

XWM_TINY = $(TARGET_DIR)/$(ROOT_DIR)/sbin/x/xwm_tiny

PROGS += $(XWM_TINY)
CLEAN += $(XWM_TINY_OBJS)

$(XWM_TINY): $(XWM_TINY_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(XWM_TINY_OBJS) -o $(XWM_TINY) $(LDFLAGS) -lgraph -lewokc -lsconf -lc -lx
