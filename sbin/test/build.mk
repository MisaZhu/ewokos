TEST_DIR = sbin/test
TEST_PROGRAM = $(TEST_DIR)/test 
PROGRAM += $(TEST_PROGRAM)

EXTRA_CLEAN += $(TEST_PROGRAM)

$(TEST_PROGRAM): $(TEST_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -I $(TEST_DIR) -c -o $(TEST_DIR)/test.o $(TEST_DIR)/test.c
	$(LD) -Ttext=100 $(TEST_DIR)/*.o lib/libewok.a -o $@
	cp $(TEST_DIR)/test.c sbin/sbin
	mv $(TEST_PROGRAM) sbin/sbin
	rm -f $(TEST_DIR)/*.o $(TEST_DIR)/dev/*.o
