#ifndef EXT3_JBD_H
#define EXT3_JBD_H

#include <stdint.h>
#include <stddef.h>

/*
 * Minimal JBD (Journaling Block Device) layer, the journal format used by
 * Linux ext3.  All journal structures are BIG-ENDIAN on disk (unlike the
 * ext2/ext3 structures themselves) and are accessed byte-wise through
 * helpers in jbd.c, so no unaligned/aliasing tricks are needed.
 *
 * On-disk layout of the journal (a regular file inside the fs, usually
 * inode 8): block 0 holds the journal superblock, blocks [s_first,
 * s_maxlen) are the cyclic log.  A transaction is laid out as:
 *
 *   [revoke blocks...] [descriptor] [journaled data blocks...]
 *   [descriptor] [data...] [commit block]
 *
 * The journal superblock's s_start/s_sequence pair delimit the range of
 * committed transactions still needing replay; s_start == 0 means the log
 * is empty.  We commit at most one transaction at a time and checkpoint
 * (write the journaled blocks back home) immediately after the commit
 * block is durable, so recovery only ever replays the last transaction.
 */

#define JBD_MAGIC_NUMBER	0xc03b3998U

/* journal block types */
#define JBD_DESCRIPTOR_BLOCK	1
#define JBD_COMMIT_BLOCK	2
#define JBD_SUPERBLOCK_V1	3
#define JBD_SUPERBLOCK_V2	4
#define JBD_REVOKE_BLOCK	5

/* journal block tag flags */
#define JBD_FLAG_ESCAPE		0x01	/* on-disk block is escaped */
#define JBD_FLAG_SAME_UUID	0x02	/* block has same uuid as previous */
#define JBD_FLAG_LAST_TAG	0x08	/* last tag in this descriptor block */

/* journal (v2) incompatible features */
#define JBD_FEATURE_INCOMPAT_REVOKE	0x0001

#define JBD_MIN_JOURNAL_BLOCKS	1024

/* fixed part of every journal block header */
#define JBD_HEADER_SIZE	12
/* { be32 blocknr, be32 flags } tag without uuid/64bit extensions */
#define JBD_TAG_SIZE	8
/* revoke block: 12 byte header + be32 count */
#define JBD_REVOKE_HEADER_SIZE	16

#define JBD_MAX_BLOCK_SIZE 4096

typedef int32_t (*jbd_read_block_func_t)(int32_t block, void* buf);
typedef int32_t (*jbd_write_block_func_t)(int32_t block, const void* buf);
typedef int32_t (*jbd_flush_func_t)(void);

/* translate a journal-file block index (0..maxlen-1) to an fs block number */
typedef int32_t (*jbd_map_func_t)(void* ctx, uint32_t jblock, uint32_t* fs_block);

/* one transaction handed to jbd_commit(): block numbers + contents */
typedef struct jbd_txn {
	const uint32_t* blocks;		/* fs block numbers */
	char* const* datas;		/* block_size bytes each */
	uint32_t count;
	const uint32_t* revokes;	/* fs blocks to revoke */
	uint32_t revoke_count;
} jbd_txn_t;

typedef struct jbd {
	/* raw fs-block IO for journal file blocks */
	jbd_read_block_func_t read_block;
	jbd_write_block_func_t write_block;
	jbd_flush_func_t flush;		/* optional barrier between phases */
	jbd_map_func_t map;		/* journal block index -> fs block */
	void* ctx;			/* opaque, ext3_t* */

	uint32_t block_size;		/* == fs block size */
	uint32_t first;			/* first log block (after jsb) */
	uint32_t maxlen;		/* total journal file blocks */
	uint32_t format_version;	/* 1 or 2 */

	/* log state (kept in RAM, mirrored into the jsb on disk) */
	uint32_t head;			/* next free log block for writes */
	uint32_t next_sequence;		/* sequence id of the next txn */

	/* recovery stats */
	uint32_t recovered_txns;
	uint32_t recovered_blocks;

	/* revoke list for the transaction being built (ext3 layer appends) */
	uint32_t* revoke_list;
	uint32_t revoke_num;
	uint32_t revoke_cap;

	/* raw journal superblock image (block_size bytes) */
	char sb[JBD_MAX_BLOCK_SIZE];
} jbd_t;

/* load the journal superblock and validate the geometry */
int32_t jbd_load(jbd_t* j, jbd_read_block_func_t read_block, jbd_write_block_func_t write_block,
		jbd_flush_func_t flush, jbd_map_func_t map, void* ctx);

/* replay committed transactions found in the log, then reset the log.
 * Writes recovered blocks straight to their home locations. */
int32_t jbd_recover(jbd_t* j);

/* write a transaction to the log and update the jsb so the txn is
 * replayable.  Returns 0 once the commit block + jsb are durable;
 * the caller should then checkpoint and call jbd_checkpoint_done(). */
int32_t jbd_commit(jbd_t* j, const jbd_txn_t* txn);

/* mark the log empty (all transactions checkpointed) */
int32_t jbd_checkpoint_done(jbd_t* j);

/* revoke bookkeeping for the txn being built */
int32_t jbd_revoke(jbd_t* j, uint32_t block);
void jbd_cancel_revoke(jbd_t* j, uint32_t block);
void jbd_clear_revokes(jbd_t* j);

void jbd_free(jbd_t* j);

#endif
