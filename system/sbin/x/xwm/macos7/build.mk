XWM_MACOS7_OBJS = $(ROOT_DIR)/sbin/x/xwm/macos7/xwm.o

XWM_MACOS7 = $(TARGET_DIR)/$(ROOT_DIR)/sbin/x/xwm_macos7

PROGS += $(XWM_MACOS7)
CLEAN += $(XWM_MACOS7_OBJS)

$(XWM_MACOS7): $(XWM_MACOS7_OBJS) $(LIB_OBJS)
	$(LD) -Ttext=100 $(XWM_MACOS7_OBJS) -o $(XWM_MACOS7) $(LDFLAGS) -lgraph -lewokc -lsconf -lc -lx
