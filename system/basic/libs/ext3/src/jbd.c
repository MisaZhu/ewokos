#include <ext3/jbd.h>
#include <ext3/ext3head.h>
#include <stdlib.h>
#include <string.h>

/*
 * Journal superblock offsets (all fields big-endian on disk):
 *
 *   0x00 h_magic, h_blocktype, h_sequence     (12 byte block header)
 *   0x0C s_blocksize, s_maxlen, s_first
 *   0x18 s_sequence, s_start
 *   0x20 s_errno
 *   0x24 s_feature_compat, s_feature_incompat, s_feature_ro_compat (v2 only)
 *   0x30 s_uuid[16], s_nr_users, s_dynsuper, s_max_transaction,
 *   0x48 s_max_trans_data, ...
 */
#define JSB_OFF_BLOCKSIZE	0x0C
#define JSB_OFF_MAXLEN		0x10
#define JSB_OFF_FIRST		0x14
#define JSB_OFF_SEQUENCE	0x18
#define JSB_OFF_START		0x1C
#define JSB_OFF_ERRNO		0x20
#define JSB_OFF_INCOMPAT	0x28

/* compile-time guards: the raw jsb image must fit a max-size block */
typedef char jbd_sb_fits[(sizeof(((jbd_t*)0)->sb) >= JBD_MAX_BLOCK_SIZE) ? 1 : -1];
typedef char jbd_block_size_match[(JBD_MAX_BLOCK_SIZE == EXT3_MAX_BLOCK_SIZE) ? 1 : -1];

#define JBD_MAX_TAGS_PER_BLOCK	(JBD_MAX_BLOCK_SIZE / JBD_TAG_SIZE)

static uint32_t jbd_get_be32(const char* buf, uint32_t off) {
	const uint8_t* p = (const uint8_t*)buf + off;
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
		((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void jbd_set_be32(char* buf, uint32_t off, uint32_t v) {
	uint8_t* p = (uint8_t*)buf + off;
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

static uint32_t jbd_log_capacity(const jbd_t* j) {
	/* usable log blocks: [first, maxlen) */
	return j->maxlen - j->first;
}

/* absolute log position for the i-th block written after `from` */
static uint32_t jbd_log_wrap(const jbd_t* j, uint32_t from, uint32_t i) {
	uint32_t cap = jbd_log_capacity(j);
	uint32_t off = (from - j->first) + i;
	return j->first + (off % cap);
}

static int32_t jbd_log_read(jbd_t* j, uint32_t jblock, char* buf) {
	uint32_t fs_block = 0;
	if(jblock >= j->maxlen)
		return -1;
	if(j->map(j->ctx, jblock, &fs_block) != 0)
		return -1;
	return j->read_block((int32_t)fs_block, buf);
}

static int32_t jbd_log_write(jbd_t* j, uint32_t jblock, const char* buf) {
	uint32_t fs_block = 0;
	if(jblock >= j->maxlen)
		return -1;
	if(j->map(j->ctx, jblock, &fs_block) != 0)
		return -1;
	return j->write_block((int32_t)fs_block, buf);
}

static void jbd_barrier(jbd_t* j) {
	if(j->flush != NULL)
		j->flush();
}

static void jbd_write_header(char* buf, uint32_t blocktype, uint32_t sequence) {
	jbd_set_be32(buf, 0, JBD_MAGIC_NUMBER);
	jbd_set_be32(buf, 4, blocktype);
	jbd_set_be32(buf, 8, sequence);
}

static int32_t jbd_check_header(const char* buf, uint32_t* blocktype, uint32_t* sequence) {
	if(jbd_get_be32(buf, 0) != JBD_MAGIC_NUMBER)
		return -1;
	*blocktype = jbd_get_be32(buf, 4);
	*sequence = jbd_get_be32(buf, 8);
	if(*blocktype == 0 || *blocktype > JBD_REVOKE_BLOCK)
		return -1;
	return 0;
}

/* ---------- journal superblock ---------- */

static int32_t jbd_write_super(jbd_t* j) {
	return jbd_log_write(j, 0, j->sb);
}

int32_t jbd_load(jbd_t* j, jbd_read_block_func_t read_block, jbd_write_block_func_t write_block,
		jbd_flush_func_t flush, jbd_map_func_t map, void* ctx) {
	uint32_t blocksize, maxlen, first, start, sequence, blocktype, incompat;

	memset(j, 0, sizeof(jbd_t));
	j->read_block = read_block;
	j->write_block = write_block;
	j->flush = flush;
	j->map = map;
	j->ctx = ctx;

	if(j->read_block == NULL || j->write_block == NULL || j->map == NULL)
		return -1;

	/* journal superblock lives in journal block 0; read it directly:
	 * jbd_log_read() bounds-checks against j->maxlen, which is only
	 * known AFTER parsing the jsb itself */
	{
		uint32_t sb_fs_block = 0;
		if(j->map(j->ctx, 0, &sb_fs_block) != 0)
			return -1;
		if(j->read_block((int32_t)sb_fs_block, j->sb) != 0)
			return -1;
	}

	if(jbd_check_header(j->sb, &blocktype, &sequence) != 0)
		return -1;
	if(blocktype != JBD_SUPERBLOCK_V1 && blocktype != JBD_SUPERBLOCK_V2)
		return -1;
	j->format_version = blocktype - JBD_SUPERBLOCK_V1 + 1;

	blocksize = jbd_get_be32(j->sb, JSB_OFF_BLOCKSIZE);
	maxlen = jbd_get_be32(j->sb, JSB_OFF_MAXLEN);
	first = jbd_get_be32(j->sb, JSB_OFF_FIRST);
	start = jbd_get_be32(j->sb, JSB_OFF_START);

	if(blocksize != EXT3_MIN_BLOCK_SIZE && blocksize != 2048 && blocksize != EXT3_MAX_BLOCK_SIZE)
		return -1;
	if(blocksize > JBD_MAX_BLOCK_SIZE)
		return -1;
	j->block_size = blocksize;

	if(maxlen < JBD_MIN_JOURNAL_BLOCKS)
		return -1;
	if(first == 0 || first >= maxlen || first > 8) /* log starts right after the jsb */
		return -1;
	j->maxlen = maxlen;
	j->first = first;

	if(j->format_version >= 2) {
		incompat = jbd_get_be32(j->sb, JSB_OFF_INCOMPAT);
		/* only plain (v1-style) journals are supported: no 64bit tags,
		 * no checksums, no fast-commit */
		if(incompat & ~JBD_FEATURE_INCOMPAT_REVOKE)
			return -1;
	}

	if(start == 0) {
		/* empty log: next txn uses sequence+1 (kernel convention) */
		j->head = j->first;
		j->next_sequence = jbd_get_be32(j->sb, JSB_OFF_SEQUENCE) + 1;
	}
	else {
		if(start < j->first || start >= j->maxlen)
			return -1;
		j->head = start; /* fixed up by recovery */
		j->next_sequence = jbd_get_be32(j->sb, JSB_OFF_SEQUENCE);
	}
	return 0;
}

/* ---------- recovery ---------- */

typedef struct jbd_revoke_rec {
	uint32_t block;
	uint32_t sequence;
	struct jbd_revoke_rec* next;
} jbd_revoke_rec_t;

/* replay context shared by the two recovery passes */
typedef struct jbd_replay {
	jbd_t* j;
	jbd_revoke_rec_t* revokes;
	char* data;		/* scratch block */
	char* desc;		/* descriptor block under scan */
	uint32_t last_seq;	/* sequence of last complete txn */
	uint32_t txns;
	uint32_t blocks;
	uint32_t fs_blocks;	/* s_blocks_count bound for tag sanity */
} jbd_replay_t;

static int32_t jbd_revoke_test(const jbd_replay_t* rp, uint32_t block, uint32_t sequence) {
	const jbd_revoke_rec_t* r = rp->revokes;
	while(r != NULL) {
		if(r->block == block && r->sequence >= sequence)
			return 1;
		r = r->next;
	}
	return 0;
}

static int32_t jbd_revoke_add(jbd_replay_t* rp, uint32_t block, uint32_t sequence) {
	jbd_revoke_rec_t* r = (jbd_revoke_rec_t*)malloc(sizeof(jbd_revoke_rec_t));
	if(r == NULL)
		return -1;
	r->block = block;
	r->sequence = sequence;
	r->next = rp->revokes;
	rp->revokes = r;
	return 0;
}

static void jbd_revoke_free(jbd_replay_t* rp) {
	jbd_revoke_rec_t* r = rp->revokes;
	while(r != NULL) {
		jbd_revoke_rec_t* next = r->next;
		free(r);
		r = next;
	}
	rp->revokes = NULL;
}

/*
 * Scan the log once.  In PASS_REVOKE only revoke records are collected.
 * In PASS_REPLAY complete transactions (those with a commit block) are
 * applied: descriptor tags are buffered until the commit block shows up,
 * then each tagged journal block is copied to its fs home location,
 * honouring escapes and revokes.  Mirrors the kernel's do_one_pass().
 */
typedef enum { JBD_PASS_REVOKE = 0, JBD_PASS_REPLAY = 1 } jbd_pass_t;

static int32_t jbd_do_one_pass(jbd_replay_t* rp, jbd_pass_t pass) {
	jbd_t* j = rp->j;
	uint32_t next = jbd_get_be32(j->sb, JSB_OFF_START);
	uint32_t sequence = jbd_get_be32(j->sb, JSB_OFF_SEQUENCE);
	uint32_t scanned = 0;
	uint32_t cap = jbd_log_capacity(j);

	/* buffered tags of the transaction being scanned (replay pass) */
	uint32_t tag_blocks[JBD_MAX_TAGS_PER_BLOCK];
	uint32_t tag_log_blocks[JBD_MAX_TAGS_PER_BLOCK];
	uint32_t tag_flags[JBD_MAX_TAGS_PER_BLOCK];
	uint32_t tag_num = 0;

	while(scanned < cap) {
		uint32_t blocktype = 0, hdr_seq = 0;
		if(jbd_log_read(j, next, rp->desc) != 0)
			break;
		if(jbd_check_header(rp->desc, &blocktype, &hdr_seq) != 0)
			break;
		if(hdr_seq != sequence)
			break;

		if(blocktype == JBD_REVOKE_BLOCK) {
			uint32_t count = jbd_get_be32(rp->desc, JBD_HEADER_SIZE);
			uint32_t entries;
			if(count < JBD_REVOKE_HEADER_SIZE || count > j->block_size)
				break;
			entries = (count - JBD_REVOKE_HEADER_SIZE) / 4;
			for(uint32_t i = 0; i < entries; i++) {
				uint32_t blk = jbd_get_be32(rp->desc, JBD_REVOKE_HEADER_SIZE + i * 4);
				if(pass == JBD_PASS_REVOKE) {
					if(jbd_revoke_add(rp, blk, sequence) != 0)
						return -1;
				}
			}
			next = jbd_log_wrap(j, next, 1);
			scanned++;
			continue;
		}

		if(blocktype == JBD_DESCRIPTOR_BLOCK) {
			uint32_t tags_per_block = (j->block_size - JBD_HEADER_SIZE) / JBD_TAG_SIZE;
			uint32_t tagp = JBD_HEADER_SIZE;
			uint32_t io_block = next;
			uint32_t used = 0;

			while(tagp + JBD_TAG_SIZE <= j->block_size && used < tags_per_block) {
				uint32_t blk = jbd_get_be32(rp->desc, tagp);
				uint32_t flags = jbd_get_be32(rp->desc, tagp + 4);
				io_block = jbd_log_wrap(j, io_block, 1);

				if(pass == JBD_PASS_REPLAY && used < JBD_MAX_TAGS_PER_BLOCK) {
					tag_blocks[used] = blk;
					tag_log_blocks[used] = io_block;
					tag_flags[used] = flags;
					tag_num = used + 1;
				}
				used++;

				tagp += JBD_TAG_SIZE;
				if((flags & JBD_FLAG_SAME_UUID) == 0)
					tagp += 16; /* uuid follows the tag */
				if(flags & JBD_FLAG_LAST_TAG)
					break;
			}

			/* skip over the data blocks referenced by this descriptor */
			next = jbd_log_wrap(j, next, 1 + used);
			scanned += 1 + used;
			continue;
		}

		if(blocktype == JBD_COMMIT_BLOCK) {
			if(pass == JBD_PASS_REPLAY) {
				for(uint32_t i = 0; i < tag_num; i++) {
					uint32_t blk = tag_blocks[i];
					if(blk == 0 || blk >= rp->fs_blocks)
						continue;
					if(jbd_revoke_test(rp, blk, sequence))
						continue;
					if(jbd_log_read(j, tag_log_blocks[i], rp->data) != 0)
						return -1;
					/* an escaped block was stored with the magic zeroed */
					if(tag_flags[i] & JBD_FLAG_ESCAPE)
						jbd_set_be32(rp->data, 0, JBD_MAGIC_NUMBER);
					if(j->write_block((int32_t)blk, rp->data) != 0)
						return -1;
					rp->blocks++;
				}
				tag_num = 0;
				rp->txns++;
			}
			rp->last_seq = sequence;
			sequence++;
			next = jbd_log_wrap(j, next, 1);
			scanned++;
			continue;
		}

		/* unknown block type: end of valid log */
		break;
	}
	return 0;
}

int32_t jbd_recover(jbd_t* j) {
	jbd_replay_t rp;
	uint32_t start = jbd_get_be32(j->sb, JSB_OFF_START);
	int32_t ret = 0;

	if(start == 0) {
		/* nothing to replay */
		j->head = j->first;
		j->next_sequence = jbd_get_be32(j->sb, JSB_OFF_SEQUENCE) + 1;
		return 0;
	}

	memset(&rp, 0, sizeof(rp));
	rp.j = j;
	rp.fs_blocks = ((ext3_t*)j->ctx)->super.s_blocks_count;
	rp.data = (char*)malloc(JBD_MAX_BLOCK_SIZE);
	rp.desc = (char*)malloc(JBD_MAX_BLOCK_SIZE);
	if(rp.data == NULL || rp.desc == NULL) {
		free(rp.data);
		free(rp.desc);
		return -1;
	}

	/* pass 1: collect revoke records, pass 2: replay */
	if(jbd_do_one_pass(&rp, JBD_PASS_REVOKE) != 0 ||
			jbd_do_one_pass(&rp, JBD_PASS_REPLAY) != 0)
		ret = -1;

	j->recovered_txns = rp.txns;
	j->recovered_blocks = rp.blocks;
	if(ret == 0) {
		/* the replay itself checkpointed every complete txn: make the
		 * recovered writes durable before declaring the log empty */
		jbd_barrier(j);
		j->head = j->first;
		if(rp.txns > 0)
			j->next_sequence = rp.last_seq + 1;
		else
			j->next_sequence = jbd_get_be32(j->sb, JSB_OFF_SEQUENCE) + 1;
		jbd_set_be32(j->sb, JSB_OFF_START, 0);
		jbd_set_be32(j->sb, JSB_OFF_SEQUENCE, j->next_sequence - 1);
		if(jbd_write_super(j) != 0)
			ret = -1;
	}

	jbd_revoke_free(&rp);
	free(rp.data);
	free(rp.desc);
	return ret;
}

/* ---------- revoke bookkeeping for the running txn ---------- */

int32_t jbd_revoke(jbd_t* j, uint32_t block) {
	if(j->revoke_num == j->revoke_cap) {
		uint32_t cap = (j->revoke_cap == 0) ? 16 : (j->revoke_cap * 2);
		uint32_t* list = (uint32_t*)realloc(j->revoke_list, cap * sizeof(uint32_t));
		if(list == NULL)
			return -1;
		j->revoke_list = list;
		j->revoke_cap = cap;
	}
	for(uint32_t i = 0; i < j->revoke_num; i++) {
		if(j->revoke_list[i] == block)
			return 0;
	}
	j->revoke_list[j->revoke_num++] = block;
	return 0;
}

void jbd_cancel_revoke(jbd_t* j, uint32_t block) {
	for(uint32_t i = 0; i < j->revoke_num; i++) {
		if(j->revoke_list[i] == block) {
			j->revoke_list[i] = j->revoke_list[j->revoke_num - 1];
			j->revoke_num--;
			return;
		}
	}
}

void jbd_clear_revokes(jbd_t* j) {
	j->revoke_num = 0;
}

void jbd_free(jbd_t* j) {
	if(j == NULL)
		return;
	free(j->revoke_list);
	j->revoke_list = NULL;
	j->revoke_num = 0;
	j->revoke_cap = 0;
}

/* ---------- commit ---------- */

/* log blocks needed by a txn: revokes + descriptors + data + commit */
static uint32_t jbd_txn_log_blocks(jbd_t* j, uint32_t revoke_count, uint32_t block_count) {
	uint32_t entries_per_revoke = (j->block_size - JBD_REVOKE_HEADER_SIZE) / 4;
	uint32_t tags_per_desc = (j->block_size - JBD_HEADER_SIZE) / JBD_TAG_SIZE;
	uint32_t revoke_blocks = (revoke_count + entries_per_revoke - 1) / entries_per_revoke;
	uint32_t desc_blocks = (block_count + tags_per_desc - 1) / tags_per_desc;
	return revoke_blocks + desc_blocks + block_count + 1;
}

int32_t jbd_commit(jbd_t* j, const jbd_txn_t* txn) {
	uint32_t sequence = j->next_sequence;
	uint32_t entries_per_revoke = (j->block_size - JBD_REVOKE_HEADER_SIZE) / 4;
	uint32_t tags_per_desc = (j->block_size - JBD_HEADER_SIZE) / JBD_TAG_SIZE;
	uint32_t revoke_blocks = (txn->revoke_count + entries_per_revoke - 1) / entries_per_revoke;
	uint32_t total = jbd_txn_log_blocks(j, txn->revoke_count, txn->count);
	char* buf;
	int32_t ret = -1;

	if(txn->count == 0 && txn->revoke_count == 0)
		return 0;
	if(total > jbd_log_capacity(j))
		return -1;

	buf = (char*)malloc(j->block_size);
	if(buf == NULL)
		return -1;

	uint32_t pos = j->head;

	/* 1. revoke records */
	for(uint32_t r = 0; r < revoke_blocks; r++) {
		uint32_t first = r * entries_per_revoke;
		uint32_t n = txn->revoke_count - first;
		if(n > entries_per_revoke)
			n = entries_per_revoke;
		memset(buf, 0, j->block_size);
		jbd_write_header(buf, JBD_REVOKE_BLOCK, sequence);
		jbd_set_be32(buf, JBD_HEADER_SIZE, JBD_REVOKE_HEADER_SIZE + n * 4);
		for(uint32_t i = 0; i < n; i++)
			jbd_set_be32(buf, JBD_REVOKE_HEADER_SIZE + i * 4, txn->revokes[first + i]);
		if(jbd_log_write(j, pos, buf) != 0)
			goto out;
		pos = jbd_log_wrap(j, pos, 1);
	}

	/* 2. descriptors + journaled data blocks */
	for(uint32_t b = 0; b < txn->count; ) {
		uint32_t n = txn->count - b;
		uint32_t tagp = JBD_HEADER_SIZE;
		if(n > tags_per_desc)
			n = tags_per_desc;

		memset(buf, 0, j->block_size);
		jbd_write_header(buf, JBD_DESCRIPTOR_BLOCK, sequence);
		for(uint32_t i = 0; i < n; i++) {
			uint32_t flags = JBD_FLAG_SAME_UUID;
			const char* data = txn->datas[b + i];
			/* escape: a data block starting with the journal magic is
			 * stored with the magic zeroed out */
			if(jbd_get_be32(data, 0) == JBD_MAGIC_NUMBER)
				flags |= JBD_FLAG_ESCAPE;
			if(i == n - 1)
				flags |= JBD_FLAG_LAST_TAG;
			jbd_set_be32(buf, tagp, txn->blocks[b + i]);
			jbd_set_be32(buf, tagp + 4, flags);
			tagp += JBD_TAG_SIZE;
		}
		if(jbd_log_write(j, pos, buf) != 0)
			goto out;
		pos = jbd_log_wrap(j, pos, 1);

		for(uint32_t i = 0; i < n; i++) {
			/* recompute the escape flag from the data itself: the
			 * descriptor that carried it was in `buf` and is already
			 * overwritten with the previous data block */
			const char* data = txn->datas[b + i];
			uint32_t flags = JBD_FLAG_SAME_UUID;
			if(jbd_get_be32(data, 0) == JBD_MAGIC_NUMBER)
				flags |= JBD_FLAG_ESCAPE;
			memcpy(buf, data, j->block_size);
			if(flags & JBD_FLAG_ESCAPE)
				jbd_set_be32(buf, 0, 0);
			if(jbd_log_write(j, pos, buf) != 0)
				goto out;
			pos = jbd_log_wrap(j, pos, 1);
		}
		b += n;
	}

	/* the whole txn must be on disk before the commit record */
	jbd_barrier(j);

	/* 3. commit record */
	memset(buf, 0, j->block_size);
	jbd_write_header(buf, JBD_COMMIT_BLOCK, sequence);
	if(jbd_log_write(j, pos, buf) != 0)
		goto out;
	pos = jbd_log_wrap(j, pos, 1);
	jbd_barrier(j);

	/* 4. publish the txn in the journal superblock: from now on the
	 * transaction is replayable until the checkpoint completes */
	jbd_set_be32(j->sb, JSB_OFF_START, j->head);
	jbd_set_be32(j->sb, JSB_OFF_SEQUENCE, sequence);
	if(jbd_write_super(j) != 0)
		goto out;
	jbd_barrier(j);

	j->head = pos;
	j->next_sequence = sequence + 1;
	ret = 0;
out:
	free(buf);
	return ret;
}

int32_t jbd_checkpoint_done(jbd_t* j) {
	if(jbd_get_be32(j->sb, JSB_OFF_START) == 0)
		return 0;
	jbd_set_be32(j->sb, JSB_OFF_START, 0);
	jbd_set_be32(j->sb, JSB_OFF_SEQUENCE, j->next_sequence - 1);
	j->head = j->first;
	return jbd_write_super(j);
}
