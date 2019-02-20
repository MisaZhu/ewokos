LS_DIR = bin/ls
LS_PROGRAM = build/ls 
PROGRAM += $(LS_PROGRAM)

EXTRA_CLEAN += $(LS_PROGRAM) $(LS_DIR)/*.o

$(LS_PROGRAM): $(LS_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -c -o $(LS_DIR)/ls.o $(LS_DIR)/ls.c
	$(LD) -Ttext=100 $(LS_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(LS_PROGRAM)
