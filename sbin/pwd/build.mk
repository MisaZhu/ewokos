PWD_DIR = sbin/pwd
PWD_PROGRAM = $(PWD_DIR)/pwd
PROGRAM += $(PWD_PROGRAM)

EXTRA_CLEAN += $(PWD_PROGRAM)

$(PWD_PROGRAM): $(PWD_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -I $(PWD_DIR) -c -o $(PWD_DIR)/pwd.o $(PWD_DIR)/pwd.c
	$(LD) -Ttext=100 $(PWD_DIR)/*.o lib/libewok.a -o $@
	mv $(PWD_PROGRAM) sbin/sbin
	rm -f $(PWD_DIR)/*.o $(PWD_DIR)/dev/*.o
