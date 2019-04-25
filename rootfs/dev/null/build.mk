NULL_DIR = dev/null
NULL_PROGRAM = build/sbin/dev/null
PROGRAM += $(NULL_PROGRAM)

EXTRA_CLEAN += $(NULL_PROGRAM) $(NULL_DIR)/*.o

$(NULL_PROGRAM): $(NULL_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(NULL_DIR) -c -o $(NULL_DIR)/null.o $(NULL_DIR)/null.c
	$(LD) -Ttext=100 $(NULL_DIR)/null.o lib/libewoklibc.a $(COMMON_OBJ) -o $(NULL_PROGRAM)
