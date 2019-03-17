CONSOLED_DIR = dev/consoled
CONSOLED_PROGRAM = build/consoled
PROGRAM += $(CONSOLED_PROGRAM)

EXTRA_CLEAN += $(CONSOLED_PROGRAM) $(CONSOLED_DIR)/*.o

$(CONSOLED_PROGRAM): $(CONSOLED_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(CONSOLED_DIR) -c -o $(CONSOLED_DIR)/consoled.o $(CONSOLED_DIR)/consoled.c
	$(LD) -Ttext=100 $(CONSOLED_DIR)/consoled.o lib/libewoklibc.a $(COMMON_OBJ) -o $(CONSOLED_PROGRAM)
