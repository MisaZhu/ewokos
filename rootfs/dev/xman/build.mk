XMAN_DIR = dev/xman
XMAN_PROGRAM = build/sbin/dev/xman
PROGRAM += $(XMAN_PROGRAM)

EXTRA_CLEAN += $(XMAN_PROGRAM) $(XMAN_DIR)/*.o

$(XMAN_PROGRAM): $(XMAN_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(XMAN_DIR) -c -o $(XMAN_DIR)/xman.o $(XMAN_DIR)/xman.c
	$(LD) -Ttext=100 $(XMAN_DIR)/xman.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XMAN_PROGRAM)
