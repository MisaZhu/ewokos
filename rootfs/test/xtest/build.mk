XTEST_DIR = test/xtest
XTEST_PROGRAM = build/usr/bin/xtest 
PROGRAM += $(XTEST_PROGRAM)

EXTRA_CLEAN += $(XTEST_PROGRAM) $(XTEST_DIR)/*.o

$(XTEST_PROGRAM): $(XTEST_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(XTEST_DIR)/xtest.o $(XTEST_DIR)/xtest.c
	$(LD) -Ttext=100 $(XTEST_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XTEST_PROGRAM)
