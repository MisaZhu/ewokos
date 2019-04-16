SDCARD_DIR = dev/sdcard
SDCARD_PROGRAM = build/sbin/dev/sdcard 
PROGRAM += $(SDCARD_PROGRAM)

EXTRA_CLEAN += $(SDCARD_PROGRAM) $(SDCARD_DIR)/*.o

$(SDCARD_PROGRAM): $(SDCARD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(SDCARD_DIR) -c -o $(SDCARD_DIR)/sdcard.o $(SDCARD_DIR)/sdcard.c
	$(LD) -Ttext=100 $(SDCARD_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(SDCARD_PROGRAM)
