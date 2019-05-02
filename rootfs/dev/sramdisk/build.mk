SRAMDISK_DIR = dev/sramdisk
SRAMDISK_PROGRAM = build/sbin/dev/sramdisk 
PROGRAM += $(SRAMDISK_PROGRAM)

EXTRA_CLEAN += $(SRAMDISK_PROGRAM) $(SRAMDISK_DIR)/*.o

$(SRAMDISK_PROGRAM): $(SRAMDISK_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(SRAMDISK_DIR) -c -o $(SRAMDISK_DIR)/sramdisk.o $(SRAMDISK_DIR)/sramdisk.c
	$(LD) -Ttext=100 $(SRAMDISK_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(SRAMDISK_PROGRAM)
