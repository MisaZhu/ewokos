KEYB_DIR = test/keyb
KEYB_PROGRAM = build/usr/bin/keyb 
PROGRAM += $(KEYB_PROGRAM)

EXTRA_CLEAN += $(KEYB_PROGRAM) $(KEYB_DIR)/*.o

$(KEYB_PROGRAM): $(KEYB_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(KEYB_DIR)/keyb.o $(KEYB_DIR)/keyb.c
	$(LD) -Ttext=100 $(KEYB_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(KEYB_PROGRAM)
