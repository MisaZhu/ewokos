PIPE_TEST_DIR = test/pipe
PIPE_TEST_PROGRAM = build/usr/bin/pipe_test 
PROGRAM += $(PIPE_TEST_PROGRAM)

EXTRA_CLEAN += $(PIPE_TEST_PROGRAM) $(PIPE_TEST_DIR)/*.o

$(PIPE_TEST_PROGRAM): $(PIPE_TEST_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(PIPE_TEST_DIR)/pipe_test.o $(PIPE_TEST_DIR)/pipe_test.c
	$(LD) -Ttext=100 $(PIPE_TEST_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(PIPE_TEST_PROGRAM)
