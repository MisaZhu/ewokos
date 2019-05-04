DUMP_DIR = bin/dump
DUMP_PROGRAM = build/bin/dump 
PROGRAM += $(DUMP_PROGRAM)

EXTRA_CLEAN += $(DUMP_PROGRAM) $(DUMP_DIR)/*.o

$(DUMP_PROGRAM): $(DUMP_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(DUMP_DIR)/dump.o $(DUMP_DIR)/dump.c
	$(LD) -Ttext=100 $(DUMP_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(DUMP_PROGRAM)
