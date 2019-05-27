XLAUNCHER_DIR = x/xlauncher
XLAUNCHER_PROGRAM = build/bin/x/xlauncher
PROGRAM += $(XLAUNCHER_PROGRAM)

EXTRA_CLEAN += $(XLAUNCHER_PROGRAM) $(XLAUNCHER_DIR)/*.o

$(XLAUNCHER_PROGRAM): $(XLAUNCHER_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(XLAUNCHER_DIR) -c -o $(XLAUNCHER_DIR)/xlauncher.o $(XLAUNCHER_DIR)/xlauncher.c
	$(LD) -Ttext=100 $(XLAUNCHER_DIR)/xlauncher.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XLAUNCHER_PROGRAM)
