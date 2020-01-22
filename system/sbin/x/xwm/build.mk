XWM_OBJS = $(ROOT_DIR)/sbin/x/xwm/xwm.o

XWM = $(TARGET_DIR)/$(ROOT_DIR)/sbin/x/xwm

PROGS += $(XWM)
CLEAN += $(XWM_OBJS)

$(XWM): $(XWM_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(XWM_OBJS) $(LIB_OBJS) -o $(XWM) $(LDFLAGS)
