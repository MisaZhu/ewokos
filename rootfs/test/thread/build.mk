THREAD_DIR = test/thread
THREAD_PROGRAM = build/usr/bin/test_thread 
PROGRAM += $(THREAD_PROGRAM)

EXTRA_CLEAN += $(THREAD_PROGRAM) $(THREAD_DIR)/*.o

$(THREAD_PROGRAM): $(THREAD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(THREAD_DIR)/test_thread.o $(THREAD_DIR)/test_thread.c
	$(LD) -Ttext=100 $(THREAD_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(THREAD_PROGRAM)
