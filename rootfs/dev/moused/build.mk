MOUSED_DIR = dev/moused
MOUSED_PROGRAM = build/sbin/dev/moused
PROGRAM += $(MOUSED_PROGRAM)

EXTRA_CLEAN += $(MOUSED_PROGRAM) $(MOUSED_DIR)/*.o

$(MOUSED_PROGRAM): $(MOUSED_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(MOUSED_DIR) -c -o $(MOUSED_DIR)/moused.o $(MOUSED_DIR)/moused.c
	$(LD) -Ttext=100 $(MOUSED_DIR)/moused.o lib/libewoklibc.a $(COMMON_OBJ) -o $(MOUSED_PROGRAM)
