SHELL_PROGRAM = sbin/shell/shell 
PROGRAM += $(SHELL_PROGRAM)

EXTRA_CLEAN += $(SHELL_PROGRAM)

$(SHELL_PROGRAM): sbin/shell/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $@.o $<
	$(LD) -Ttext=100 $@.o lib/libewok.a -o $@
	mv $(SHELL_PROGRAM) sbin/sbin
	rm -f $@.o
