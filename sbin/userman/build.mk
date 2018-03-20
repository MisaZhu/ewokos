USERMAN_DIR = sbin/userman
USERMAN_PROGRAM = sbin/sbin/userman 
PROGRAM += $(USERMAN_PROGRAM)

EXTRA_CLEAN += $(USERMAN_PROGRAM) $(USERMAN_DIR)/*.o

$(USERMAN_PROGRAM): $(USERMAN_DIR)/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -I $(USERMAN_DIR) -c -o $(USERMAN_DIR)/userman.o $(USERMAN_DIR)/userman.c
	$(LD) -Ttext=100 $(USERMAN_DIR)/*.o lib/libewok.a -o $(USERMAN_PROGRAM)
