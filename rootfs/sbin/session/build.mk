SESSION_DIR = sbin/session
SESSION_PROGRAM = build/sbin/session 
PROGRAM += $(SESSION_PROGRAM)

EXTRA_CLEAN += $(SESSION_PROGRAM) $(SESSION_DIR)/*.o

$(SESSION_PROGRAM): $(SESSION_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(SESSION_DIR)/session.o $(SESSION_DIR)/session.c
	$(LD) -Ttext=100 $(SESSION_DIR)/*.o lib/libewoklibc.a  $(COMMON_OBJ) -o $(SESSION_PROGRAM)
