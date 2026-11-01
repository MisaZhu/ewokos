#include <sd/sd.h>
#include <ewoksys/klog.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <bsp/bsp_sd.h>
#include <string.h>
#include <unistd.h>
//#include <ext2/ext2fs.h>
#include <ext3/ext3fs.h>

#if defined(__has_include)
# if __has_include(<q8b_platform.h>)
#  include <q8b_platform.h>
#  define INIT_HAS_Q8B_BOOTFS 1
# endif
#endif

#define SD_BUFFER_SIZE (1024*1024*8)

static int32_t ext_sd_read_blocks(int32_t block, void* buf, uint32_t count) {
    return sd_read_blocks(block, buf, count);
}

#ifdef INIT_HAS_Q8B_BOOTFS
#define Q8B_BOOTFS_VA 0x60000000ULL
static uint8_t* _bootfs_va;
static uint32_t _bootfs_size;
static int32_t _bootfs_pid = -1;
static uint32_t _bootfs_block_size = 1024;

static void bootfs_unmap(void) {
    if (_bootfs_va != NULL && _bootfs_size != 0)
        (void)syscall3(SYS_MEM_UNMAP, (ewokos_addr_t)Q8B_BOOTFS_VA,
                (ewokos_addr_t)_bootfs_size, 0);
    _bootfs_va = NULL;
    _bootfs_size = 0;
    _bootfs_pid = -1;
    _bootfs_block_size = 1024;
}

static int bootfs_map(void) {
    sys_info_t info;
    const q8b_platform_data_t* pd;
    int32_t pid = (int32_t)getpid();
    if (_bootfs_va != NULL && _bootfs_pid == pid)
        return 0;
    if (sys_get_sys_info(&info) != 0) {
        klog("init: bootfs sysinfo failed\n");
        return -1;
    }
    pd = (const q8b_platform_data_t*)info.platform_data;
    if (pd->rd.phy_base == 0 || pd->rd.size == 0 ||
            (pd->rd.flags & Q8B_RD_FLAG_BOOT_ONLY) == 0) {
        return -1;
    }
    ewokos_addr_t map_ret = syscall3(SYS_MEM_MAP, (ewokos_addr_t)Q8B_BOOTFS_VA,
            (ewokos_addr_t)pd->rd.phy_base, (ewokos_addr_t)pd->rd.size);
    if (map_ret != (ewokos_addr_t)Q8B_BOOTFS_VA) {
        klog("init: bootfs map failed ret=0x%llx\n",
                (unsigned long long)map_ret);
        return -1;
    }
    _bootfs_va = (uint8_t*)(uintptr_t)Q8B_BOOTFS_VA;
    _bootfs_size = pd->rd.size;
    _bootfs_pid = pid;
    _bootfs_block_size = 1024;
    return 0;
}

/* ext3's generic userspace reader addresses filesystem blocks, while the
 * RAMFS backing store is a raw image with no partition offset.  The initial
 * superblock is always read as a 1 KiB block; after that read, switch to the
 * block size advertised by s_log_block_size for all metadata and file IO. */
static int32_t bootfs_read_block(int32_t block, void* buf) {
    uint64_t off;
    uint32_t bytes = _bootfs_block_size;
    if (block < 0 || buf == NULL || bytes == 0)
        return -1;
    off = (uint64_t)(uint32_t)block * bytes;
    if (off + bytes < off || off + bytes > _bootfs_size)
        return -1;
    memcpy(buf, _bootfs_va + off, bytes);
    if (block == 1 && bytes == 1024) {
        const uint8_t* sb = (const uint8_t*)buf;
        uint32_t log = (uint32_t)sb[24];
        if (log <= 2)
            _bootfs_block_size = 1024U << log;
    }
    return 0;
}

static int32_t bootfs_write_block(int32_t block, const void* buf) {
    (void)block;
    (void)buf;
    return -1;
}

static void* read_bootfs(const char* fname, off_t* size) {
    ext3_t ext3;
    void* ret;
    int32_t init_ret;
    if (bootfs_map() != 0)
        return NULL;
    init_ret = ext3_init_ro_ex2(&ext3, bootfs_read_block,
            NULL, bootfs_write_block, NULL, NULL, SD_BUFFER_SIZE);
    if (init_ret != 0) {
        klog("init: bootfs ext3 init failed %d (%s)\n", init_ret, fname);
        bootfs_unmap();
        return NULL;
    }
    ret = ext3_readfile(&ext3, fname, size);
    ext3_quit(&ext3);
    /* The returned file is heap-owned.  The firmware image is only a source
     * for this read and must not remain mapped into a process that may later
     * exec a persistent-rootfs daemon. */
    bootfs_unmap();
    return ret;
}
#endif

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
static void* read_ext3(const char* fname, off_t* size) {
    ext3_t ext3;
    int32_t r = ext3_init_ro_ex2(&ext3, sd_read, ext_sd_read_blocks, sd_write, NULL,
            NULL, SD_BUFFER_SIZE);
    if(r != 0) {
        klog("init: ext3_init_ex2=%d (%s)\n", r, fname);
        return NULL;
    }
    void* ret = ext3_readfile(&ext3, fname, size);
    ext3_quit(&ext3);
    return ret;
}

/* same contract as the kernel's read_fs: probe the partition and use the
 * matching reader (ext3 preferred when a usable journal is present) */
void* read_fs(const char* fname, off_t* size) {
#ifdef INIT_HAS_Q8B_BOOTFS
    void* boot = read_bootfs(fname, size);
    if (boot != NULL)
        return boot;
#endif
    if (bsp_sd_init() != 0)
        return NULL;
    return read_ext3(fname, size);
    //return read_ext2(fname, size);
}
