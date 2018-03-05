KSERV_PROGRAM = sbin/kserv/kserv 
PROGRAM += $(KSERV_PROGRAM)

EXTRA_CLEAN += $(KSERV_PROGRAM)

$(KSERV_PROGRAM): sbin/kserv/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $@.o $<
	$(LD) -Ttext=100 $@.o lib/libewok.a -o $@
	mv $(KSERV_PROGRAM) sbin/sbin
	rm -f $@.o
