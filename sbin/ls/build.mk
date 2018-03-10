LS_DIR = sbin/ls
LS_PROGRAM = $(LS_DIR)/ls 
PROGRAM += $(LS_PROGRAM)

EXTRA_CLEAN += $(LS_PROGRAM)

$(LS_PROGRAM): $(LS_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -I $(LS_DIR) -c -o $(LS_DIR)/ls.o $(LS_DIR)/ls.c
	$(LD) -Ttext=100 $(LS_DIR)/*.o lib/libewok.a -o $@
	mv $(LS_PROGRAM) sbin/sbin
	rm -f $(LS_DIR)/*.o $(LS_DIR)/dev/*.o
