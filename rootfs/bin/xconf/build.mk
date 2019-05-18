XCONF_DIR = bin/xconf
XCONF_PROGRAM = build/bin/xconf 
PROGRAM += $(XCONF_PROGRAM)

EXTRA_CLEAN += $(XCONF_PROGRAM) $(XCONF_DIR)/*.o

$(XCONF_PROGRAM): $(XCONF_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(XCONF_DIR)/xconf.o $(XCONF_DIR)/xconf.c
	$(LD) -Ttext=100 $(XCONF_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XCONF_PROGRAM)
