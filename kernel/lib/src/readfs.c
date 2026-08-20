#include <readfs.h>
//#include <ext2read.h>
#include <ext3read.h>

/*
 * Boot-time rootfs file reader: probe the rootfs partition once and
 * dispatch to the matching reader.  ext3 is preferred when the partition
 * carries a usable journal; anything else keeps the legacy ext2 reader.
 */
void* read_fs(const char* fname, int32_t* sz) {
	return read_ext3(fname, sz);
	/*if(ext3_probe_fs() == EXT3_PROBE_EXT3)
		return read_ext3(fname, sz);
	return read_ext2(fname, sz);
    */
}
