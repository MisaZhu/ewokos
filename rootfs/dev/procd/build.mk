PROCD_DIR = dev/procd
PROCD_PROGRAM = build/procd
PROGRAM += $(PROCD_PROGRAM)

EXTRA_CLEAN += $(PROCD_PROGRAM) $(PROCD_DIR)/*.o

$(PROCD_PROGRAM): $(PROCD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(PROCD_DIR) -c -o $(PROCD_DIR)/procd.o $(PROCD_DIR)/procd.c
	$(LD) -Ttext=100 $(PROCD_DIR)/procd.o lib/libewoklibc.a $(COMMON_OBJ) -o $(PROCD_PROGRAM)
