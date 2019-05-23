XSERV_DIR = dev/xserv
XSERV_PROGRAM = build/sbin/dev/xserv
PROGRAM += $(XSERV_PROGRAM)

EXTRA_CLEAN += $(XSERV_PROGRAM) $(XSERV_DIR)/*.o

$(XSERV_PROGRAM): $(XSERV_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build
	$(CC) $(CFLAGS) -I $(XSERV_DIR) -c -o $(XSERV_DIR)/xserv.o $(XSERV_DIR)/xserv.c
	$(LD) -Ttext=100 $(XSERV_DIR)/xserv.o lib/libewoklibc.a $(COMMON_OBJ) -o $(XSERV_PROGRAM)
