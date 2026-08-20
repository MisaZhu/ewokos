#ifndef EXT3_FS_READ_H
#define EXT3_FS_READ_H

#include <stdint.h>

/* ext3_probe_fs() results: filesystem type of the rootfs partition */
#define EXT3_PROBE_NONE 0
#define EXT3_PROBE_EXT2 2
#define EXT3_PROBE_EXT3 3

/*
 * Probe the rootfs partition's filesystem type (result cached, the card
 * layout does not change during boot).  ext3 is checked first: a
 * filesystem with a usable internal journal is classified ext3, any
 * other ext-compatible filesystem is ext2.
 */
int32_t ext3_probe_fs(void);

/*
 * Read a whole file from an ext3 rootfs partition (kmalloc'd buffer,
 * caller frees).  Read-only: the journal is neither written nor replayed.
 */
void* read_ext3(const char* fname, int32_t* size);

#endif
