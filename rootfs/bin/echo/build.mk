ECHO_DIR = bin/echo
ECHO_PROGRAM = build/bin/echo 
PROGRAM += $(ECHO_PROGRAM)

EXTRA_CLEAN += $(ECHO_PROGRAM) $(ECHO_DIR)/*.o

$(ECHO_PROGRAM): $(ECHO_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(ECHO_DIR)/echo.o $(ECHO_DIR)/echo.c
	$(LD) -Ttext=100 $(ECHO_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(ECHO_PROGRAM)
