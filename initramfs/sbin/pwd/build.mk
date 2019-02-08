PWD_DIR = sbin/pwd
PWD_PROGRAM = sbin/sbin/pwd 
PROGRAM += $(PWD_PROGRAM)

EXTRA_CLEAN += $(PWD_PROGRAM) $(PWD_DIR)/*.o

$(PWD_PROGRAM): $(PWD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(PWD_DIR)/pwd.o $(PWD_DIR)/pwd.c
	$(LD) -Ttext=100 $(PWD_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(PWD_PROGRAM)
