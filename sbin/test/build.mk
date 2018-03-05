TEST_PROGRAM = sbin/test/test 
PROGRAM += $(TEST_PROGRAM)

EXTRA_CLEAN += $(TEST_PROGRAM)

$(TEST_PROGRAM): sbin/test/*.c lib/libewok.a
	mkdir -p sbin/sbin
	$(CC) $(CFLAGS) -c -o $@.o $<
	$(LD) -Ttext=100 $@.o lib/libewok.a -o $@
	mv $(TEST_PROGRAM) sbin/sbin
	rm -f $@.o
