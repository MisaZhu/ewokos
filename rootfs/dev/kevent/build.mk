KEVENTD_DIR = dev/kevent
KEVENTD_PROGRAM = build/sbin/dev/keventd
PROGRAM += $(KEVENTD_PROGRAM)

EXTRA_CLEAN += $(KEVENTD_PROGRAM) $(KEVENTD_DIR)/*.o

$(KEVENTD_PROGRAM): $(KEVENTD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(KEVENTD_DIR) -c -o $(KEVENTD_DIR)/keventd.o $(KEVENTD_DIR)/keventd.c
	$(LD) -Ttext=100 $(KEVENTD_DIR)/keventd.o lib/libewoklibc.a $(COMMON_OBJ) -o $(KEVENTD_PROGRAM)
