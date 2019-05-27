XPANEL_DIR = x/xpanel
XPANEL_PROGRAM = build/bin/x/xpanel
PROGRAM += $(XPANEL_PROGRAM)

EXTRA_CLEAN += $(XPANEL_PROGRAM) $(XPANEL_DIR)/*.o

$(XPANEL_PROGRAM): $(XPANEL_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(XPANEL_DIR) -c -o $(XPANEL_DIR)/xpanel.o $(XPANEL_DIR)/xpanel.c
	$(LD) -Ttext=100 $(XPANEL_DIR)/xpanel.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XPANEL_PROGRAM)
