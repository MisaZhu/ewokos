#include <sd/sd.h>
//#include <ext2/ext2fs.h>
#include <ext3/ext3fs.h>
#include <bsp/bsp_sd.h>

#define SD_BUFFER_SIZE (1024*1024*8)

static int32_t ext_sd_read_blocks(int32_t block, void* buf, uint32_t count) {
    return sd_read_blocks(block, buf, count);
}

/*static void* read_ext2(const char* fname, int32_t* size) {
    ext2_t ext2;
    if(ext2_init_ex(&ext2, sd_read, ext2_sd_read_blocks, NULL, SD_BUFFER_SIZE) != 0) {
        return NULL;
    }
    void* ret = ext2_readfile(&ext2, fname, size);
    ext2_quit(&ext2);
    return ret;
}
*/

/* ext3 path through the ext3 library: the journal is recovered on init and
 * the fs is committed/closed cleanly on quit; only the read API is used. */
static void* read_ext3(const char* fname, int32_t* size) {
    ext3_t ext3;
    if(ext3_init_ex2(&ext3, sd_read, ext_sd_read_blocks, sd_write, NULL,
            bsp_sd_flush, SD_BUFFER_SIZE) != 0) {
        return NULL;
    }
    void* ret = ext3_readfile(&ext3, fname, size);
    ext3_quit(&ext3);
    return ret;
}

/* same contract as the kernel's read_fs: probe the partition and use the
 * matching reader (ext3 preferred when a usable journal is present) */
void* read_fs(const char* fname, int32_t* size) {
    return read_ext3(fname, size);
    //return read_ext2(fname, size);
}