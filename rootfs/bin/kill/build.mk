KILL_DIR = bin/kill
KILL_PROGRAM = build/kill 
PROGRAM += $(KILL_PROGRAM)

EXTRA_CLEAN += $(KILL_PROGRAM) $(KILL_DIR)/*.o

$(KILL_PROGRAM): $(KILL_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -c -o $(KILL_DIR)/kill.o $(KILL_DIR)/kill.c
	$(LD) -Ttext=100 $(KILL_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(KILL_PROGRAM)
