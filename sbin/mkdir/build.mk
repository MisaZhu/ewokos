MKDIR_DIR = sbin/mkdir
MKDIR_PROGRAM = $(MKDIR_DIR)/mkdir 
PROGRAM += $(MKDIR_PROGRAM)

EXTRA_CLEAN += $(MKDIR_PROGRAM)

$(MKDIR_PROGRAM): $(MKDIR_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -I $(MKDIR_DIR) -c -o $(MKDIR_DIR)/mkdir.o $(MKDIR_DIR)/mkdir.c
	$(LD) -Ttext=100 $(MKDIR_DIR)/*.o lib/libewok.a -o $@
	mv $(MKDIR_PROGRAM) sbin/sbin
	rm -f $(MKDIR_DIR)/*.o $(MKDIR_DIR)/dev/*.o
