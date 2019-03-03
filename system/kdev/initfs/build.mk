VFS_DIR = kdev/initfs
VFS_PROGRAM = build/initfs 
PROGRAM += $(VFS_PROGRAM)

EXTRA_CLEAN += $(VFS_PROGRAM) $(VFS_DIR)/*.o

$(VFS_PROGRAM): $(VFS_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/initfs.o $(VFS_DIR)/initfs.c
	$(LD) -Ttext=100 $(VFS_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(VFS_PROGRAM)
