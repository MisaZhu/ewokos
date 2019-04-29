MOUSE_DIR = test/mouse
MOUSE_PROGRAM = build/usr/bin/mouse 
PROGRAM += $(MOUSE_PROGRAM)

EXTRA_CLEAN += $(MOUSE_PROGRAM) $(MOUSE_DIR)/*.o

$(MOUSE_PROGRAM): $(MOUSE_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(MOUSE_DIR)/mouse.o $(MOUSE_DIR)/mouse.c
	$(LD) -Ttext=100 $(MOUSE_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(MOUSE_PROGRAM)
