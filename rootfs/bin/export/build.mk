EXPORT_DIR = bin/export
EXPORT_PROGRAM = build/bin/export 
PROGRAM += $(EXPORT_PROGRAM)

EXTRA_CLEAN += $(EXPORT_PROGRAM) $(EXPORT_DIR)/*.o

$(EXPORT_PROGRAM): $(EXPORT_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(EXPORT_DIR)/export.o $(EXPORT_DIR)/export.c
	$(LD) -Ttext=100 $(EXPORT_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(EXPORT_PROGRAM)
