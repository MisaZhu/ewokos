TTYD_DIR = kdev/ttyd
TTYD_PROGRAM = build/ttyd
PROGRAM += $(TTYD_PROGRAM)

EXTRA_CLEAN += $(TTYD_PROGRAM) $(TTYD_DIR)/*.o

$(TTYD_PROGRAM): $(TTYD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(TTYD_DIR) -c -o $(TTYD_DIR)/ttyd.o $(TTYD_DIR)/ttyd.c
	$(LD) -Ttext=100 $(TTYD_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(TTYD_PROGRAM)
