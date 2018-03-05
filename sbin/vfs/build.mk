VFS_PROGRAM = sbin/vfs/vfs 
PROGRAM += $(VFS_PROGRAM)

EXTRA_CLEAN += $(VFS_PROGRAM)

$(VFS_PROGRAM): sbin/vfs/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $@.o $<
	$(LD) -Ttext=100 $@.o lib/libewok.a -o $@
	mv $(VFS_PROGRAM) sbin/sbin
	rm -f $@.o
