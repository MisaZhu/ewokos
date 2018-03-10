PS_DIR = sbin/ps
PS_PROGRAM = $(PS_DIR)/ps 
PROGRAM += $(PS_PROGRAM)

EXTRA_CLEAN += $(PS_PROGRAM)

$(PS_PROGRAM): $(PS_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -I $(PS_DIR) -c -o $(PS_DIR)/ps.o $(PS_DIR)/ps.c
	$(LD) -Ttext=100 $(PS_DIR)/*.o lib/libewok.a -o $@
	mv $(PS_PROGRAM) sbin/sbin
	rm -f $(PS_DIR)/*.o $(PS_DIR)/dev/*.o
