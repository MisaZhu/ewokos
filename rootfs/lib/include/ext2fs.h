#ifndef EXT2_FS_H
#define EXT2_FS_H

#include <ext2.h>

typedef struct {
	int32_t iblock;
	int32_t nblocks;
	int32_t ninodes;
	int32_t bmap;
	int32_t imap;

	read_block_func_t read_block;
	write_block_func_t write_block;
} ext2_t;

int32_t ext2_init(ext2_t* ext2, read_block_func_t read_block, write_block_func_t write_block);

int32_t ext2_rm_child(ext2_t* ext2, INODE *ip, const char *name);

int32_t ext2_read(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset);

int32_t ext2_write(ext2_t* ext2, INODE* node, char *data, int32_t nbytes, int32_t offset);

INODE* get_node(ext2_t* ext2, int32_t ino, char* buf);

void put_node(ext2_t* ext2, int32_t ino, INODE *node);

int32_t ext2_create_dir(ext2_t* ext2, INODE* father_inp, const char *base);

int32_t ext2_create_file(ext2_t* ext2, INODE* father_inp, const char *base);

#endif
