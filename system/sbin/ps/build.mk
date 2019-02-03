PS_DIR = sbin/ps
PS_PROGRAM = sbin/sbin/ps 
PROGRAM += $(PS_PROGRAM)

EXTRA_CLEAN += $(PS_PROGRAM) $(PS_DIR)/*.o

$(PS_PROGRAM): $(PS_DIR)/*.c $(KERNEL_LIB)/libewok.a lib/libewoklibc.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(PS_DIR)/ps.o $(PS_DIR)/ps.c
	$(LD) -Ttext=100 $(PS_DIR)/*.o lib/libewoklibc.a $(KERNEL_LIB)/libewok.a -o $(PS_PROGRAM)
