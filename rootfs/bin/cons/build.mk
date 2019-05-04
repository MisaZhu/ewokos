CONS_DIR = bin/cons
CONS_PROGRAM = build/bin/cons 
PROGRAM += $(CONS_PROGRAM)

EXTRA_CLEAN += $(CONS_PROGRAM) $(CONS_DIR)/*.o

$(CONS_PROGRAM): $(CONS_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(CONS_DIR)/cons.o $(CONS_DIR)/cons.c
	$(LD) -Ttext=100 $(CONS_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(CONS_PROGRAM)
