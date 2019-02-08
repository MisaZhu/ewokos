TEST_DIR = sbin/test
TEST_PROGRAM = build/sbin/test 
PROGRAM += $(TEST_PROGRAM)

EXTRA_CLEAN += $(TEST_PROGRAM) $(TEST_DIR)/*.o

$(TEST_PROGRAM): $(TEST_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build/sbin
	$(CC) $(CFLAGS) -c -o $(TEST_DIR)/test.o $(TEST_DIR)/test.c
	$(LD) -Ttext=100 $(TEST_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(TEST_PROGRAM)
