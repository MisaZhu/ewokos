INIT_DIR = sbin/init
INIT_PROGRAM = sbin/sbin/init 
PROGRAM += $(INIT_PROGRAM)

EXTRA_CLEAN += $(INIT_PROGRAM) $(INIT_DIR)/*.o

$(INIT_PROGRAM): $(INIT_DIR)/*.c ../lib/libewok.a lib/libewoklibc.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(INIT_DIR)/init.o $(INIT_DIR)/init.c
	$(LD) -Ttext=100 $(INIT_DIR)/*.o lib/libewoklibc.a  ../lib/libewok.a -o $(INIT_PROGRAM)
