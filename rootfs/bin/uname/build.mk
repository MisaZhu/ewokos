UNAME_DIR = bin/uname
UNAME_PROGRAM = build/bin/uname 
PROGRAM += $(UNAME_PROGRAM)

EXTRA_CLEAN += $(UNAME_PROGRAM) $(UNAME_DIR)/*.o

$(UNAME_PROGRAM): $(UNAME_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(UNAME_DIR)/uname.o $(UNAME_DIR)/uname.c
	$(LD) -Ttext=100 $(UNAME_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(UNAME_PROGRAM)
