FBD_DIR = kdev/fbd
FBD_PROGRAM = build/fbd
PROGRAM += $(FBD_PROGRAM)

EXTRA_CLEAN += $(FBD_PROGRAM) $(FBD_DIR)/*.o

$(FBD_PROGRAM): $(FBD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(FBD_DIR) -c -o $(FBD_DIR)/fbd.o $(FBD_DIR)/fbd.c
	$(LD) -Ttext=100 $(FBD_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(FBD_PROGRAM)
