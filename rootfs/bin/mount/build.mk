MOUNT_DIR = bin/mount
MOUNT_PROGRAM = build/bin/mount 
PROGRAM += $(MOUNT_PROGRAM)

EXTRA_CLEAN += $(MOUNT_PROGRAM) $(MOUNT_DIR)/*.o

$(MOUNT_PROGRAM): $(MOUNT_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(MOUNT_DIR)/mount.o $(MOUNT_DIR)/mount.c
	$(LD) -Ttext=100 $(MOUNT_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(MOUNT_PROGRAM)
