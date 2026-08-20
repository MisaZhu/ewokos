#ifndef EXT3_FS_H
#define EXT3_FS_H

#include <ext3/ext3head.h>
#include <ext3/jbd.h>

/*
 * ext3 filesystem library.
 *
 * The API mirrors the ext2 library so a driver (sdfsd) can serve both
 * filesystems: ext3_init() reads the superblock and, when the filesystem
 * carries a valid journal (EXT3_FEATURE_COMPAT_HAS_JOURNAL set, internal
 * journal inode), it runs journal recovery and enables journaling.
 * A plain ext2 filesystem is mounted in ext2 compatible mode: identical
 * behaviour to the old ext2 library (direct writes, no journal).
 */

/* ext3_init* return codes */
#define EXT3_ERR_IO		-1	/* IO / format error */
#define EXT3_ERR_JOURNAL	-2	/* journal declared but unusable */

/* ext3_probe() results */
#define EXT3_PROBE_NONE		0
#define EXT3_PROBE_EXT2		2
#define EXT3_PROBE_EXT3		3

int32_t ext3_init(ext3_t* ext3, read_block_func_t read_block, write_block_func_t write_block, uint32_t buffer_size);
int32_t ext3_init_ex(ext3_t* ext3, read_block_func_t read_block, read_blocks_func_t read_blocks,
		write_block_func_t write_block, write_blocks_func_t write_blocks, uint32_t buffer_size);
int32_t ext3_init_ex2(ext3_t* ext3, read_block_func_t read_block, read_blocks_func_t read_blocks,
		write_block_func_t write_block, write_blocks_func_t write_blocks,
		flush_func_t flush, uint32_t buffer_size);

/*
 * Probe the filesystem type on the raw block device.  ext3 (with journal)
 * is checked first; only a filesystem without a journal is classified as
 * ext2.  NOTE: like ext3_init, this expects the device block size to be
 * the fs block size; it temporarily switches it to 1024 to read the
 * superblock.  Returns EXT3_PROBE_EXT3, EXT3_PROBE_EXT2 or
 * EXT3_PROBE_NONE (not an ext filesystem).
 */
int32_t ext3_probe(read_block_func_t read_block);

void ext3_quit(ext3_t* ext3);

/*
 * Commit the current transaction: journal all dirty metadata blocks,
 * write the commit record, then checkpoint them to their home locations.
 * In ext2 compatible mode it just flushes the dirty blocks to disk.
 * sdfsd should call this after metadata-modifying operations.
 */
int32_t ext3_commit(ext3_t* ext3);

int32_t ext3_rmdir(ext3_t* ext3, const char* fname);

int32_t ext3_unlink(ext3_t* ext3, const char* fname);

int32_t ext3_read(ext3_t* ext3, EXT3_INODE* node, char *buf, int32_t nbytes, int32_t offset);

int32_t ext3_read_block(ext3_t* ext3, EXT3_INODE* node, char *buf, int32_t nbytes, int32_t offset);

int32_t ext3_write(ext3_t* ext3, EXT3_INODE* node, const char *data, int32_t nbytes, int32_t offset);

int32_t ext3_truncate(ext3_t* ext3, uint32_t ino, EXT3_INODE* node);

uint32_t ext3_ino_by_fname(ext3_t* ext3, const char* fname, EXT3_DIR_T* dirp);

int32_t ext3_node_by_fname(ext3_t* ext3, const char* fname, EXT3_INODE* node);

int32_t ext3_node_by_ino(ext3_t* ext3, uint32_t ino, EXT3_INODE* node);

int32_t ext3_put_node(ext3_t* ext3, uint32_t ino, EXT3_INODE *node);

int32_t ext3_create_dir(ext3_t* ext3, uint32_t father_ino, EXT3_INODE* father_inp, const char *base,
		uint16_t uid, uint16_t gid, uint16_t mode);

int32_t ext3_create_file(ext3_t* ext3, uint32_t father_ino, EXT3_INODE* father_inp, const char *base,
		uint16_t uid, uint16_t gid, uint16_t mode);

void*   ext3_readfile(ext3_t* ext3, const char* fname, int32_t* size);

#endif
