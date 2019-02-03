TEST_DIR = sbin/test
TEST_PROGRAM = sbin/sbin/test 
PROGRAM += $(TEST_PROGRAM)

EXTRA_CLEAN += $(TEST_PROGRAM) $(TEST_DIR)/*.o

$(TEST_PROGRAM): $(TEST_DIR)/*.c ../lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(TEST_DIR)/test.o $(TEST_DIR)/test.c
	$(LD) -Ttext=100 $(TEST_DIR)/*.o ../lib/libewok.a -o $(TEST_PROGRAM)
