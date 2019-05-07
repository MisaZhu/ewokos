GREP_DIR = bin/grep
GREP_PROGRAM = build/bin/grep 
PROGRAM += $(GREP_PROGRAM)

EXTRA_CLEAN += $(GREP_PROGRAM) $(GREP_DIR)/*.o

$(GREP_PROGRAM): $(GREP_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(GREP_DIR)/grep.o $(GREP_DIR)/grep.c
	$(LD) -Ttext=100 $(GREP_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(GREP_PROGRAM)
