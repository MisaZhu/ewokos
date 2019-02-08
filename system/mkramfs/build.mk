MKRAMFS = mkramfs/mkramfs
EXTRA_CLEAN += $(MKRAMFS)

$(MKRAMFS): mkramfs/mkramfs.c
	gcc mkramfs/mkramfs.c -g -o mkramfs/mkramfs
