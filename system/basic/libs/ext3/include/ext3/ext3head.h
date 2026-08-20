#ifndef EXT3_FS_HEAD_H
#define EXT3_FS_HEAD_H

#include <stdint.h>
#include <stddef.h>

/*
 * ext3 on-disk structures.
 *
 * ext3 is a strict superset of ext2: the first 200 bytes of the superblock
 * are identical, and the area ext2 leaves reserved holds the journal
 * related fields (journal inode, uuid, ...).  Everything below follows the
 * layout used by Linux ext3 / e2fsprogs so images are interchangeable.
 */

#define EXT3_FT_FILE 1
#define EXT3_FT_DIR  2

/* i_mode file-type bits (what external ext2/ext3 tools/OSes look at;
 * the in-tree rebuild only reads the dirent file_type byte). */
#define EXT3_S_IFREG 0x8000
#define EXT3_S_IFDIR 0x4000

#define EXT3_SUPER_MAGIC 0xEF53

/* compatible features (safe to ignore) */
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL   0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR      0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE  0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX     0x0020

/* incompatible features (must understand to mount) */
#define EXT2_FEATURE_INCOMPAT_FILETYPE    0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER     0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008

/* readonly-compatible features */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002

/* s_state values */
#define EXT2_VALID_FS 1
#define EXT2_ERROR_FS 2

typedef struct ext3_super_block {
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
	uint32_t	s_first_ino; 		/* First non-reserved inode */
	uint16_t	s_inode_size; 		/* size of inode structure */
	uint16_t	s_block_group_nr; 	/* block group # of this superblock */
	uint32_t	s_feature_compat; 	/* compatible feature set */
	uint32_t	s_feature_incompat; 	/* incompatible feature set */
	uint32_t	s_feature_ro_compat; 	/* readonly-compatible feature set */
	uint8_t	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	/*
	 * ext3 journal fields: they live inside what ext2 calls
	 * s_reserved[] (offsets 200..1023 of the superblock).
	 */
	uint32_t	s_algorithm_usage_bitmap;	/* 200 */
	uint8_t	s_prealloc_blocks;		/* 204 */
	uint8_t	s_prealloc_dir_blocks;		/* 205 */
	uint16_t	s_reserved_gdt_blocks;		/* 206 */
	uint8_t	s_journal_uuid[16];		/* 208: uuid of journal superblock */
	uint32_t	s_journal_inum;			/* 224: journal inode number */
	uint32_t	s_journal_dev;			/* 228: journal device (external) */
	uint32_t	s_last_orphan;			/* 232: start of orphan list */
	uint32_t	s_hash_seed[4];			/* 236 */
	uint8_t	s_def_hash_version;		/* 252 */
	uint8_t	s_jnl_backup_type;		/* 253 */
	uint16_t	s_desc_size;			/* 254: group desc size (ext3, 64bit) */
	uint32_t	s_default_mount_opts;		/* 256 */
	uint32_t	s_first_meta_bg;		/* 260 */
	uint32_t	s_reserved[190];	/* 264: Padding to the end of the block */
} EXT3_SUPER;

typedef struct ext3_group_desc {
	uint32_t	bg_block_bitmap;	/* block group, Blocks bitmap block */
	uint32_t	bg_inode_bitmap;	/* Inodes bitmap block */
	uint32_t	bg_inode_table;		/* Inodes table block */
	uint16_t	bg_free_blocks_count;	/* Free blocks count */
	uint16_t	bg_free_inodes_count;	/* Free inodes count */
	uint16_t	bg_used_dirs_count;	/* Directories count */
	uint16_t	bg_pad;
	uint32_t	bg_reserved[3];
} EXT3_GD;

typedef struct ext3_inode {
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
	uint32_t	dummy;
	uint32_t	i_block[15];    /* Pointers to blocks */
	uint32_t	pad[5];         /* opaque: generation/file_acl/dir_acl/faddr/osd2 */
	uint32_t	i_date;         /* MTX date */
	uint32_t	i_time;         /* MTX time */
} EXT3_INODE;

typedef struct ext3_dir_entry_2 {
	uint32_t	inode;			/* Inode number */
	uint16_t	rec_len;		/* Directory entry length */
	uint8_t		name_len;		/* Name length */
	uint8_t		file_type;
	char		name[255];      	/* File name */
} EXT3_DIR_T;

#define EXT3_MIN_BLOCK_SIZE 1024
#define EXT3_MAX_BLOCK_SIZE 4096
#define EXT3_DEFAULT_BLOCK_SIZE EXT3_MIN_BLOCK_SIZE

typedef int32_t (*read_block_func_t)(int32_t block, void* buf);
typedef int32_t (*read_blocks_func_t)(int32_t block, void* buf, uint32_t count);
typedef int32_t (*write_block_func_t)(int32_t block, const void* buf);
typedef int32_t (*write_blocks_func_t)(int32_t block, const void* buf, uint32_t count);
typedef int32_t (*flush_func_t)(void);

/* forward declaration, see ext3/jbd.h (plain struct tag: jbd.h's typedef
 * must stay the only definition of jbd_t) */
struct jbd;

/* dirty metadata block cache (journaled writes pending commit) */
typedef struct ext3_blkcache {
	uint32_t blk;
	struct ext3_blkcache* next;
	char data[EXT3_MAX_BLOCK_SIZE];
} ext3_blkcache_t;

#define EXT3_CACHE_BUCKETS 128

typedef struct {
	int32_t group_num;
	EXT3_SUPER super;
	EXT3_GD* gds;
	uint32_t next_alloc_block;

	/* deferred metadata write-back (batched like the ext2 library until
	 * put_node/commit merge the bitmap/GDT/super writes) */
	uint8_t* dirty_gds;
	uint8_t dirty_super;

	/* raw fs-block IO (e.g. sd_read/sd_write), block size is the fs one */
	read_block_func_t read_block;
	read_blocks_func_t read_blocks;
	write_block_func_t write_block;
	write_blocks_func_t write_blocks;
	/* optional device barrier, called between journal IO phases */
	flush_func_t flush;

	/* journal state; NULL => ext2 compatible mode (no journal on fs) */
	struct jbd* journal;
	uint32_t journal_ino;
	EXT3_INODE journal_inode;
	uint32_t journal_txn_limit;	/* auto-commit threshold, in blocks */
	uint32_t recovered_txns;	/* stats filled by recovery at mount */
	uint32_t recovered_blocks;
	uint8_t error;			/* sticky: any commit/IO failure */
	uint8_t commit_pending;		/* committed txn awaits checkpoint (re)try */

	/* dirty metadata block cache, hashed by blk */
	ext3_blkcache_t* cache[EXT3_CACHE_BUCKETS];
	uint32_t cache_size;
} ext3_t;

static inline uint32_t ext3_block_size(const ext3_t* ext3) {
	return EXT3_MIN_BLOCK_SIZE << ext3->super.s_log_block_size;
}

static inline uint32_t ext3_inode_size(const ext3_t* ext3) {
	return (ext3->super.s_inode_size == 0) ? 128U : (uint32_t)ext3->super.s_inode_size;
}

static inline uint32_t ext3_indirect_entries(const ext3_t* ext3) {
	return ext3_block_size(ext3) / sizeof(uint32_t);
}

static inline uint32_t ext3_group_start_block(const ext3_t* ext3, uint32_t group_index) {
	return ext3->super.s_first_data_block + (group_index * ext3->super.s_blocks_per_group);
}

static inline uint32_t ext3_gdt_start_block(const ext3_t* ext3) {
	return ext3->super.s_first_data_block + 1;
}

static inline uint32_t ext3_has_journal(const ext3_t* ext3) {
	return ext3->journal != NULL;
}

#endif
