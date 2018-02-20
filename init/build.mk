INIT_CC = arm-none-eabi-gcc
INIT_CFLAGS = -mcpu=arm1176jz-s -marm -g0 \
         -std=c99 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf -fno-builtin-strcpy -nostdinc -nostdlib \
         -I lib/include

OBJS += init/init_img.o
INIT_PROGRAM = init/init

EXTRA_CLEAN += $(INIT_PROGRAM) init/base16 init/init_img.c

init/%: init/%.c
	$(INIT_CC) $(INIT_CFLAGS) -c -o $@.o $<
	$(LD) -Ttext=100 $@.o \
	lib/src/*.o \
 	-o $@
	rm -f $@.o

init/base16: init/base16.c
	gcc $< -o $@

init/init_img.c: init/base16 $(INIT_PROGRAM)
	rm -f $@
	echo "const char *init_img[] = " >> $@
	init/base16 < $(INIT_PROGRAM) >> $@
	echo ";" >> $@
