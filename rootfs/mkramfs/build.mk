MKRAMFS = mkramfs/mkramfs
EXTRA_CLEAN += $(MKRAMFS) ../lib/src/base16.o

SRCS = mkramfs/mkramfs.c ../lib/src/base16.c

$(MKRAMFS): $(SRCS)
	gcc $(SRCS) -I../kernel/include -I../lib/include -g -o mkramfs/mkramfs
