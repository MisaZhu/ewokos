XMAND_DIR = dev/xmand
XMAND_PROGRAM = build/sbin/dev/xmand
PROGRAM += $(XMAND_PROGRAM)

EXTRA_CLEAN += $(XMAND_PROGRAM) $(XMAND_DIR)/*.o

$(XMAND_PROGRAM): $(XMAND_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(XMAND_DIR) -c -o $(XMAND_DIR)/xmand.o $(XMAND_DIR)/xmand.c
	$(LD) -Ttext=100 $(XMAND_DIR)/xmand.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XMAND_PROGRAM)
