VFSD_DIR = sbin/vfsd
VFSD_PROGRAM = build/vfsd
PROGRAM += $(VFSD_PROGRAM)

EXTRA_CLEAN += $(VFSD_PROGRAM) $(VFSD_DIR)/*.o

$(VFSD_PROGRAM): $(VFSD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(VFSD_DIR) -c -o $(VFSD_DIR)/fstree.o $(VFSD_DIR)/fstree.c
	$(CC) $(CFLAGS) -I $(VFSD_DIR) -c -o $(VFSD_DIR)/vfsd.o $(VFSD_DIR)/vfsd.c
	$(LD) -Ttext=100 $(VFSD_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(VFSD_PROGRAM)
