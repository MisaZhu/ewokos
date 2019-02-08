UID_DIR = sbin/uid
UID_PROGRAM = sbin/sbin/uid 
PROGRAM += $(UID_PROGRAM)

EXTRA_CLEAN += $(UID_PROGRAM) $(UID_DIR)/*.o

$(UID_PROGRAM): $(UID_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(UID_DIR)/uid.o $(UID_DIR)/uid.c
	$(LD) -Ttext=100 $(UID_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(UID_PROGRAM)
