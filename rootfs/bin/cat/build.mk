CAT_DIR = bin/cat
CAT_PROGRAM = build/bin/cat 
PROGRAM += $(CAT_PROGRAM)

EXTRA_CLEAN += $(CAT_PROGRAM) $(CAT_DIR)/*.o

$(CAT_PROGRAM): $(CAT_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -c -o $(CAT_DIR)/cat.o $(CAT_DIR)/cat.c
	$(LD) -Ttext=100 $(CAT_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(CAT_PROGRAM)
