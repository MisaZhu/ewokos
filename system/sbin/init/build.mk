INIT_DIR = sbin/init
INIT_PROGRAM = build/sbin/init 
PROGRAM += $(INIT_PROGRAM)

EXTRA_CLEAN += $(INIT_PROGRAM) $(INIT_DIR)/*.o

$(INIT_PROGRAM): $(INIT_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build/sbin
	$(CC) $(CFLAGS) -c -o $(INIT_DIR)/init.o $(INIT_DIR)/init.c
	$(LD) -Ttext=100 $(INIT_DIR)/*.o lib/libewoklibc.a  $(COMMON_OBJ) -o $(INIT_PROGRAM)
