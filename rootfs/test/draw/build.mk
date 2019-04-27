DRAW_DIR = test/draw
DRAW_PROGRAM = build/usr/bin/draw 
PROGRAM += $(DRAW_PROGRAM)

EXTRA_CLEAN += $(DRAW_PROGRAM) $(DRAW_DIR)/*.o

$(DRAW_PROGRAM): $(DRAW_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(DRAW_DIR)/draw.o $(DRAW_DIR)/draw.c
	$(LD) -Ttext=100 $(DRAW_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(DRAW_PROGRAM)
