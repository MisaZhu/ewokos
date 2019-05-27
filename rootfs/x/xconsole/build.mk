XCONSOLE_DIR = x/xconsole
XCONSOLE_PROGRAM = build/bin/x/xconsole
PROGRAM += $(XCONSOLE_PROGRAM)

EXTRA_CLEAN += $(XCONSOLE_PROGRAM) $(XCONSOLE_DIR)/*.o

$(XCONSOLE_PROGRAM): $(XCONSOLE_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(XCONSOLE_DIR) -c -o $(XCONSOLE_DIR)/xconsole.o $(XCONSOLE_DIR)/xconsole.c
	$(LD) -Ttext=100 $(XCONSOLE_DIR)/xconsole.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XCONSOLE_PROGRAM)
