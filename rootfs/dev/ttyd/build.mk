TTYD_DIR = dev/ttyd
TTYD_PROGRAM = build/sbin/dev/ttyd
PROGRAM += $(TTYD_PROGRAM)

EXTRA_CLEAN += $(TTYD_PROGRAM) $(TTYD_DIR)/*.o

$(TTYD_PROGRAM): $(TTYD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(TTYD_DIR) -c -o $(TTYD_DIR)/ttyd.o $(TTYD_DIR)/ttyd.c
	$(LD) -Ttext=100 $(TTYD_DIR)/ttyd.o lib/libewoklibc.a $(COMMON_OBJ) -o $(TTYD_PROGRAM)
