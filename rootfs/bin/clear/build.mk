CLEAR_DIR = bin/clear
CLEAR_PROGRAM = build/bin/clear 
PROGRAM += $(CLEAR_PROGRAM)

EXTRA_CLEAN += $(CLEAR_PROGRAM) $(CLEAR_DIR)/*.o

$(CLEAR_PROGRAM): $(CLEAR_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(CLEAR_DIR)/clear.o $(CLEAR_DIR)/clear.c
	$(LD) -Ttext=100 $(CLEAR_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(CLEAR_PROGRAM)
