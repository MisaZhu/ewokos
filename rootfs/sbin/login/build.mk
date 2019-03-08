LOGIN_DIR = sbin/login
LOGIN_PROGRAM = build/login 
PROGRAM += $(LOGIN_PROGRAM)

EXTRA_CLEAN += $(LOGIN_PROGRAM) $(LOGIN_DIR)/*.o

$(LOGIN_PROGRAM): $(LOGIN_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I$(LOGIN_DIR) -c -o $(LOGIN_DIR)/login.o $(LOGIN_DIR)/login.c
	$(CC) $(CFLAGS) -I$(LOGIN_DIR) -c -o $(LOGIN_DIR)/userman.o $(LOGIN_DIR)/userman.c
	$(LD) -Ttext=100 $(LOGIN_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(LOGIN_PROGRAM)
