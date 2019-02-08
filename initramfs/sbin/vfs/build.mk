VFS_DIR = sbin/vfs
VFS_PROGRAM = build/sbin/vfs 
PROGRAM += $(VFS_PROGRAM)

EXTRA_CLEAN += $(VFS_PROGRAM) $(VFS_DIR)/*.o $(VFS_DIR)/dev/*.o

$(VFS_PROGRAM): $(VFS_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build/sbin
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/tree.o $(VFS_DIR)/tree.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/vfs.o $(VFS_DIR)/vfs.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/mount.o $(VFS_DIR)/mount.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/dev/dev_sramdisk.o $(VFS_DIR)/dev/dev_sramdisk.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/dev/dev_tty.o $(VFS_DIR)/dev/dev_tty.c
	$(LD) -Ttext=100 $(VFS_DIR)/*.o $(VFS_DIR)/dev/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(VFS_PROGRAM)
