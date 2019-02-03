LS_DIR = sbin/ls
LS_PROGRAM = sbin/sbin/ls 
PROGRAM += $(LS_PROGRAM)

EXTRA_CLEAN += $(LS_PROGRAM) $(LS_DIR)/*.o

$(LS_PROGRAM): $(LS_DIR)/*.c ../lib/libewok.a lib/libewoklibc.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $(LS_DIR)/ls.o $(LS_DIR)/ls.c
	$(LD) -Ttext=100 $(LS_DIR)/*.o lib/libewoklibc.a ../lib/libewok.a -o $(LS_PROGRAM)
