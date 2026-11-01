#include <readfs.h>
#include <stddef.h>
//#include <ext2read.h>
#include <ext3read.h>

/* Q8B supplies a standalone boot-only ext3 image.  Other machines leave
 * this weak hook unresolved and continue using their normal SD reader. */
extern int32_t q8b_ramdisk_read_block(int32_t block, void* buf)
        __attribute__((weak));

/*
 * Boot-time rootfs file reader: probe the rootfs partition once and
 * dispatch to the matching reader.  ext3 is preferred when the partition
 * carries a usable journal; anything else keeps the legacy ext2 reader.
 */
void* read_fs(const char* fname, int32_t* sz) {
	if(q8b_ramdisk_read_block != NULL) {
		void* boot = read_ext3_with_reader(q8b_ramdisk_read_block, 0,
				fname, sz);
		if(boot != NULL)
			return boot;
	}
	return read_ext3(fname, sz);
	/*if(ext3_probe_fs() == EXT3_PROBE_EXT3)
		return read_ext3(fname, sz);
	return read_ext2(fname, sz);
    */
}
