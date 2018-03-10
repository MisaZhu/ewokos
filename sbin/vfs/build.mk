VFS_DIR = sbin/vfs
VFS_PROGRAM = $(VFS_DIR)/vfs 
PROGRAM += $(VFS_PROGRAM)

EXTRA_CLEAN += $(VFS_PROGRAM)

$(VFS_PROGRAM): $(VFS_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/tree.o $(VFS_DIR)/tree.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/vfs.o $(VFS_DIR)/vfs.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/mount.o $(VFS_DIR)/mount.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/dev/dev_sramdisk.o $(VFS_DIR)/dev/dev_sramdisk.c
	$(CC) $(CFLAGS) -I $(VFS_DIR) -c -o $(VFS_DIR)/dev/dev_tty.o $(VFS_DIR)/dev/dev_tty.c
	$(LD) -Ttext=100 $(VFS_DIR)/*.o $(VFS_DIR)/dev/*.o lib/libewok.a -o $@
	mv $(VFS_PROGRAM) sbin/sbin
	rm -f $(VFS_DIR)/*.o $(VFS_DIR)/dev/*.o
