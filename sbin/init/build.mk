INIT_PROGRAM = sbin/init/init 
PROGRAM += $(INIT_PROGRAM)

EXTRA_CLEAN += $(INIT_PROGRAM)

$(INIT_PROGRAM): sbin/init/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $@.o $<
	$(LD) -Ttext=100 $@.o lib/libewok.a -o $@
	mv $(INIT_PROGRAM) sbin/sbin
	rm -f $@.o
