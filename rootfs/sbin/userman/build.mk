USERMAN_DIR = sbin/userman
USERMAN_PROGRAM = build/userman 
PROGRAM += $(USERMAN_PROGRAM)

EXTRA_CLEAN += $(USERMAN_PROGRAM) $(USERMAN_DIR)/*.o

$(USERMAN_PROGRAM): $(USERMAN_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(USERMAN_DIR) -c -o $(USERMAN_DIR)/userman.o $(USERMAN_DIR)/userman.c
	$(LD) -Ttext=100 $(USERMAN_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(USERMAN_PROGRAM)
