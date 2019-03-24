MKRAMFS = mkramfs/mkramfs
EXTRA_CLEAN += $(MKRAMFS)

SRCS = mkramfs/mkramfs.c

$(MKRAMFS): $(SRCS)
	gcc $(SRCS) -I../kernel/include -I../lib/include -g -o mkramfs/mkramfs
