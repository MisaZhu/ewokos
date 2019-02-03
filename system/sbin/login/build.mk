LOGIN_DIR = sbin/login
LOGIN_PROGRAM = sbin/sbin/login 
PROGRAM += $(LOGIN_PROGRAM)

EXTRA_CLEAN += $(LOGIN_PROGRAM) $(LOGIN_DIR)/*.o

$(LOGIN_PROGRAM): $(LOGIN_DIR)/*.c $(KERNEL_LIB)/libewok.a lib/libewoklibc.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(LOGIN_DIR)/login.o $(LOGIN_DIR)/login.c
	$(LD) -Ttext=100 $(LOGIN_DIR)/*.o lib/libewoklibc.a $(KERNEL_LIB)/libewok.a -o $(LOGIN_PROGRAM)
