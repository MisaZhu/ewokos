SHELL_DIR = bin/shell
SHELL_PROGRAM = build/bin/shell 
PROGRAM += $(SHELL_PROGRAM)

EXTRA_CLEAN += $(SHELL_PROGRAM) $(SHELL_DIR)/*.o

$(SHELL_PROGRAM): $(SHELL_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(SHELL_DIR)/shell.o $(SHELL_DIR)/shell.c
	$(LD) -Ttext=100 $(SHELL_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(SHELL_PROGRAM)
