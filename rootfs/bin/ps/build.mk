PS_DIR = bin/ps
PS_PROGRAM = build/ps 
PROGRAM += $(PS_PROGRAM)

EXTRA_CLEAN += $(PS_PROGRAM) $(PS_DIR)/*.o

$(PS_PROGRAM): $(PS_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -c -o $(PS_DIR)/ps.o $(PS_DIR)/ps.c
	$(LD) -Ttext=100 $(PS_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(PS_PROGRAM)
