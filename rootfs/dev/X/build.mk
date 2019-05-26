X_DIR = dev/X
X_PROGRAM = build/sbin/dev/X
PROGRAM += $(X_PROGRAM)

EXTRA_CLEAN += $(X_PROGRAM) $(X_DIR)/*.o

$(X_PROGRAM): $(X_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(X_DIR) -c -o $(X_DIR)/X.o $(X_DIR)/X.c
	$(LD) -Ttext=100 $(X_DIR)/X.o lib/libewoklibc.a $(COMMON_OBJ) -o $(X_PROGRAM)
