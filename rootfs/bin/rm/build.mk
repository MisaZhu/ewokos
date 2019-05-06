RM_DIR = bin/rm
RM_PROGRAM = build/bin/rm 
PROGRAM += $(RM_PROGRAM)

EXTRA_CLEAN += $(RM_PROGRAM) $(RM_DIR)/*.o

$(RM_PROGRAM): $(RM_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -c -o $(RM_DIR)/rm.o $(RM_DIR)/rm.c
	$(LD) -Ttext=100 $(RM_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(RM_PROGRAM)
