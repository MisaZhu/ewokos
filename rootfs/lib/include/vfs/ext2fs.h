#ifndef EXT2_FS_H
#define EXT2_FS_H

#include <types.h>

#define EXT2_FT_FILE 1 
#define EXT2_FT_DIR  2

typedef struct ext2_super_block {
	uint32_t	s_inodes_count;		/* Inodes count */
	uint32_t	s_blocks_count;		/* Blocks count */
	uint32_t	s_r_blocks_count;	/* Reserved blocks count */
	uint32_t	s_free_blocks_count;	/* Free blocks count */
	uint32_t	s_free_inodes_count;	/* Free inodes count */
	uint32_t	s_first_data_block;	/* First Data Block */
	uint32_t	s_log_block_size;	/* Block size */
	uint32_t	s_log_frag_size;	/* Fragment size */
	uint32_t	s_blocks_per_group;	/* # Blocks per group */
	uint32_t	s_frags_per_group;	/* # Fragments per group */
	uint32_t	s_inodes_per_group;	/* # Inodes per group */
	uint32_t	s_mtime;		/* Mount time */
	uint32_t	s_wtime;		/* Write time */
	uint16_t	s_mnt_count;		/* Mount count */
	uint16_t	s_max_mnt_count;	/* Maximal mount count */
	uint16_t	s_magic;		/* Magic signature */
	uint16_t	s_state;		/* File system state */
	uint16_t	s_errors;		/* Behaviour when detecting errors */
	uint16_t	s_minor_rev_level; 	/* minor revision level */
	uint32_t	s_lastcheck;		/* time of last check */
	uint32_t	s_checkinterval;	/* max. time between checks */
	uint32_t	s_creator_os;		/* OS */
	uint32_t	s_rev_level;		/* Revision level */
	uint16_t	s_def_resuid;		/* Default uid for reserved blocks */
	uint16_t	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 * 
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	uint32_t	s_first_ino; 		/* First non-reserved inode */
	uint16_t  s_inode_size; 		/* size of inode structure */
	uint16_t	s_block_group_nr; 	/* block group # of this superblock */
	uint32_t	s_feature_compat; 	/* compatible feature set */
	uint32_t	s_feature_incompat; 	/* incompatible feature set */
	uint32_t	s_feature_ro_compat; 	/* readonly-compatible feature set */
	uint8_t	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	uint32_t	s_reserved[206];	/* Padding to the end of the block */
} SUPER;

typedef struct ext2_group_desc {
	uint32_t	bg_block_bitmap;	/* Blocks bitmap block */
	uint32_t	bg_inode_bitmap;	/* Inodes bitmap block */
	uint32_t	bg_inode_table;		/* Inodes table block */
	uint16_t	bg_free_blocks_count;	/* Free blocks count */
	uint16_t	bg_free_inodes_count;	/* Free inodes count */
	uint16_t	bg_used_dirs_count;	/* Directories count */
	uint16_t	bg_pad;
	uint32_t	bg_reserved[3];
} GD;

typedef struct ext2_inode {
	uint16_t	i_mode;		/* File mode */
	uint16_t	i_uid;		/* Owner Uid */
	uint32_t	i_size;		/* Size in bytes */
	uint32_t	i_atime;	/* Access time */
	uint32_t	i_ctime;	/* Creation time */
	uint32_t	i_mtime;	/* Modification time */
	uint32_t	i_dtime;	/* Deletion Time */
	uint16_t	i_gid;		/* Group Id */
	uint16_t	i_links_count;	/* Links count */
	uint32_t	i_blocks;	/* Blocks count */
	uint32_t	i_flags;	/* File flags */
	uint32_t  dummy;
	uint32_t	i_block[15];    /* Pointers to blocks */
	uint32_t  pad[5];         /* inode size MUST be 128 bytes */
	uint32_t	i_date;         /* MTX date */
	uint32_t	i_time;         /* MTX time */
} INODE;

typedef struct ext2_dir_entry_2 {
	uint32_t	inode;			/* Inode number */
	uint16_t	rec_len;		/* Directory entry length */
	uint8_t	name_len;		/* Name length */
	uint8_t	file_type;
	char	name[255];      	/* File name */
} DIR;

#define BLOCK_SIZE 1024

typedef int32_t (*read_block_func_t)(int32_t block, char* buf);
typedef int32_t (*write_block_func_t)(int32_t block, char* buf);

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

int32_t ext2_rmdir(ext2_t* ext2, const char* fname);

int32_t ext2_unlink(ext2_t* ext2, const char* fname);

int32_t ext2_read(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset);

int32_t ext2_write(ext2_t* ext2, INODE* node, char *data, int32_t nbytes, int32_t offset);

INODE* get_node(ext2_t* ext2, int32_t ino, char* buf);

void put_node(ext2_t* ext2, int32_t ino, INODE *node);

int32_t ext2_create_dir(ext2_t* ext2, INODE* father_inp, const char *base, int32_t owner);

int32_t ext2_create_file(ext2_t* ext2, INODE* father_inp, const char *base, int32_t owner);

#endif
