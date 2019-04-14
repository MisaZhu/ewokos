TEST_SD_DIR = test/sd
TEST_SD_PROGRAM = build/test_sd 
PROGRAM += $(TEST_SD_PROGRAM)

EXTRA_CLEAN += $(TEST_SD_PROGRAM) $(TEST_SD_DIR)/*.o

$(TEST_SD_PROGRAM): $(TEST_SD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -c -o $(TEST_SD_DIR)/test_sd.o $(TEST_SD_DIR)/test_sd.c
	$(LD) -Ttext=100 $(TEST_SD_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(TEST_SD_PROGRAM)
