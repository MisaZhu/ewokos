INITFS_DIR = dev/initfs
INITFS_PROGRAM = build/initfs 
PROGRAM += $(INITFS_PROGRAM)

EXTRA_CLEAN += $(INITFS_PROGRAM) $(INITFS_DIR)/*.o

$(INITFS_PROGRAM): $(INITFS_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(INITFS_DIR) -c -o $(INITFS_DIR)/initfs.o $(INITFS_DIR)/initfs.c
	$(LD) -Ttext=100 $(INITFS_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(INITFS_PROGRAM)
