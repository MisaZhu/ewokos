SHELL_DIR = sbin/shell
SHELL_PROGRAM = sbin/sbin/shell 
PROGRAM += $(SHELL_PROGRAM)

EXTRA_CLEAN += $(SHELL_PROGRAM) $(SHELL_DIR)/*.o

$(SHELL_PROGRAM): $(SHELL_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(SHELL_DIR)/shell.o $(SHELL_DIR)/shell.c
	$(LD) -Ttext=100 $(SHELL_DIR)/*.o lib/libewok.a -o $(SHELL_PROGRAM)
