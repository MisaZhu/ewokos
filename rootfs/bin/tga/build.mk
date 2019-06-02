XTGA_DIR = bin/tga
XTGA_PROGRAM = build/bin/tga 
PROGRAM += $(XTGA_PROGRAM)

EXTRA_CLEAN += $(XTGA_PROGRAM) $(XTGA_DIR)/*.o

$(XTGA_PROGRAM): $(XTGA_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(XTGA_DIR)/tga.o $(XTGA_DIR)/tga.c
	$(LD) -Ttext=100 $(XTGA_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XTGA_PROGRAM)
