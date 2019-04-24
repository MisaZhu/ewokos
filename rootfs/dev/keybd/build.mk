KEYBD_DIR = dev/keybd
KEYBD_PROGRAM = build/sbin/dev/keybd
PROGRAM += $(KEYBD_PROGRAM)

EXTRA_CLEAN += $(KEYBD_PROGRAM) $(KEYBD_DIR)/*.o

$(KEYBD_PROGRAM): $(KEYBD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(KEYBD_DIR) -c -o $(KEYBD_DIR)/keybd.o $(KEYBD_DIR)/keybd.c
	$(LD) -Ttext=100 $(KEYBD_DIR)/keybd.o lib/libewoklibc.a $(COMMON_OBJ) -o $(KEYBD_PROGRAM)
