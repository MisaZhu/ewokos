OBJS += init/init_img.o init/loadinit.o
INIT_PROGRAM = init/init

EXTRA_CLEAN += $(INIT_PROGRAM) init/base16 init/init_img.c init/init.asm

INIT_OBJS = init/init.o

init/init: $(INIT_OBJS) lib/libewok.a
	$(LD) -Ttext=100 $(INIT_OBJS) lib/libewok.a -o $@
	$(OBJDUMP) -D $@ > $@.asm
	rm -f $(INIT_OBJS)

init/base16: init/base16.c
	gcc $< -o $@

init/init_img.c: init/base16 $(INIT_PROGRAM)
	rm -f $@
	echo "const char *init_img[] = " >> $@
	init/base16 < $(INIT_PROGRAM) >> $@
	echo ";" >> $@
