MKDIR_DIR = sbin/mkdir
MKDIR_PROGRAM = sbin/sbin/mkdir 
PROGRAM += $(MKDIR_PROGRAM)

EXTRA_CLEAN += $(MKDIR_PROGRAM) $(MKDIR_DIR)/*.o

$(MKDIR_PROGRAM): $(MKDIR_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(MKDIR_DIR)/mkdir.o $(MKDIR_DIR)/mkdir.c
	$(LD) -Ttext=100 $(MKDIR_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(MKDIR_PROGRAM)
