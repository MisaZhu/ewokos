XWM_WIN3_OBJS = $(ROOT_DIR)/sbin/x/xwm/win3/xwm.o

XWM_WIN3 = $(TARGET_DIR)/$(ROOT_DIR)/sbin/x/xwm_win3

PROGS += $(XWM_WIN3)
CLEAN += $(XWM_WIN3_OBJS)

$(XWM_WIN3): $(XWM_WIN3_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(XWM_WIN3_OBJS) -o $(XWM_WIN3) $(LDFLAGS) -lgraph -lx -lewokc -lsconf -lc
