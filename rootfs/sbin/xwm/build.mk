XWM_DIR = sbin/xwm
XWM_PROGRAM = build/sbin/xwm 
PROGRAM += $(XWM_PROGRAM)

EXTRA_CLEAN += $(XWM_PROGRAM) $(XWM_DIR)/*.o

$(XWM_PROGRAM): $(XWM_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(XWM_DIR)/xwm.o $(XWM_DIR)/xwm.c
	$(LD) -Ttext=100 $(XWM_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XWM_PROGRAM)
