#include <ext2head.h>
#include <ext2read.h>
#include <kstring.h>
#include <basic_math.h>
#include <mm/kmalloc.h>
#include <mstr.h>
#include <dev/sd.h>
#include <dev/device.h>
#include "dev/actled.h"
#include <partition.h>

#define EXT2_BLOCK_SIZE 1024
#define SECTOR_SIZE     512

static partition_t _partition;

static int32_t sd_read_sector(int32_t sector, void* buf) {
	dev_t* dev = get_dev(DEV_SD);
	if(dev == NULL) 
		return -1;

	if(dev_block_read(dev, sector) != 0)
		return -1;

	while(1) {
		sd_dev_handle(dev);
		if(dev_block_read_done(dev, buf)  == 0) {
			break;
		}
	}
	return 0;
}

static int32_t sd_read(int32_t block, void* buf) {
	dev_t* dev = get_dev(DEV_SD);
	if(dev == NULL) 
		return -1;
	int32_t n = EXT2_BLOCK_SIZE/512;
	int32_t sector = block * n + _partition.start_sector;
	char* p = (char*)buf;

	while(n > 0) {
		if(sd_read_sector(sector, p) != 0) {
			return -1;
		}
		sector++;
		p += 512;
		n--;
	}
	return 0;
}

#define PARTITION_MAX 4
static partition_t _partitions[PARTITION_MAX];

int32_t read_partition(void) {
	uint8_t sector[512];
	if(sd_read_sector(0, sector) != 0)
		return -1;
	//check magic 
	if(sector[510] != 0x55 || sector[511] != 0xAA) 
		return -1;
	uint8_t* p = sector + 0x1BE;
	for(int32_t i=0; i<PARTITION_MAX; i++) {
		memcpy(&_partitions[i], p, sizeof(partition_t));
		p += sizeof(partition_t);
//		printf("partition %d: start_sector: %d\n", i, _partitions[i].start_sector);
	}

	return 0;
}

int32_t partition_get(uint32_t id, partition_t* p) {
	if(id >= PARTITION_MAX)
		return -1;
	memcpy(p, &_partitions[id], sizeof(partition_t));
	return 0;
}

#define SHORT_NAME_MAX 64

static inline int32_t get_gd_index_by_ino(ext2_t* ext2, int32_t ino) {
	return div_u32(ino, ext2->super.s_inodes_per_group);
}

static inline int32_t get_gd_index_by_block(ext2_t* ext2, int32_t block) {
	return div_u32(block, ext2->super.s_blocks_per_group);
}

static inline int32_t get_ino_in_group(ext2_t* ext2, int32_t ino, int32_t index) {
	return ino - (index * ext2->super.s_inodes_per_group);
}

static inline int32_t get_block_in_group(ext2_t* ext2, int32_t block, int32_t index) {
	return block - (index * ext2->super.s_blocks_per_group);
}

static int32_t ext2_read(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset) {
	//(2) count = 0
	// avil = fileSize - OFT's offset // number of bytes still available in file.
	int32_t count_read = 0;
	char *cq = buf;
	int32_t avil = node->i_size - offset;
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	//(3)
	/*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
		lbk       = oftp->offset / BLOCK_SIZE;
		start_byte = oftp->offset % BLOCK_SIZE;*/	
	lbk = offset / BLOCK_SIZE;
	start_byte = offset % BLOCK_SIZE;
	if(nbytes > (BLOCK_SIZE - start_byte))
		nbytes = (BLOCK_SIZE - start_byte);
	//(5) READ
	//(5).1 direct blocks
	if(lbk < 12) {
		blk = node->i_block[lbk];
	}
	//(5).2 Indirect blocks contains 256 block number 
	else if(lbk>=12 && lbk < 256 +12){
		int32_t indirect_buf[256];
		ext2->read_block(node->i_block[12], (char*)indirect_buf);
		blk = indirect_buf[lbk-12];
	}
	//(5).3 Double indiirect blocks
	else{
		int32_t count = lbk -12 -256;
		//total blocks = count / 256
		//offset of certain block = count %256
		int32_t num = count / 256;
		int32_t pos_offset = count % 256;
		int32_t double_buf1[256];
		ext2->read_block(node->i_block[13], (char*)double_buf1);
		int32_t double_buf2[256];
		ext2->read_block(double_buf1[num], (char*)double_buf2);
		blk = double_buf2[pos_offset];
	}

	char readbuf[BLOCK_SIZE];
	ext2->read_block(blk, readbuf);
	char *cp = readbuf + start_byte;
	remain = BLOCK_SIZE - start_byte;
	//(6)
	while(remain){
		int32_t min = 0;
		if(avil <= nbytes){
			min = avil;
		}
		else{
			min = nbytes;
		}
		memcpy(cq, cp, min);
		offset += min;
		count_read += min;
		avil -= min;
		nbytes -= min;
		remain -= min;
		if(nbytes == 0 || avil == 0){
			break;
		}	
	}
	return count_read;
}	

static INODE* get_node_by_ino(ext2_t* ext2, int32_t ino, char* buf) {
	int32_t bgid = get_gd_index_by_ino(ext2, ino);
	ino = get_ino_in_group(ext2, ino, bgid);

	ext2->read_block(ext2->gds[bgid].bg_inode_table + 	((ino-1)/8), buf);
	return ((INODE *)buf) + ((ino-1) % 8);
}

static int32_t search(ext2_t* ext2, INODE *ip, const char *name) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;
	char buf[BLOCK_SIZE];

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			ext2->read_block(ip->i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;
			while (cp < &buf[BLOCK_SIZE]){
				if(dp->name_len == 0)
					break;
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if (strcmp(dp->name, name) == 0 ){
					return dp->inode;
				}
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	return -1;
}

#define MAX_DIR_DEPTH 32
static int32_t split_fname(const char* filename, str_t* name[]) {
	int32_t u, depth;
	depth = 0;

	char hold[SHORT_NAME_MAX];
	while(1) {
		u = 0;
		if(*filename == '/') filename++; //skip '/'

		while(*filename != '/') {
			hold[u] = *filename;
			u++;
			filename++;
			if(*filename == 0 || u >= (SHORT_NAME_MAX-1))
				break;
		}
		hold[u] = 0;
		name[depth] = str_new(hold);
		depth++;
		if(depth >= MAX_DIR_DEPTH)
			break;
		if(*filename != 0)
			filename++;
		else
			break;
	}
	return depth;
}

static int32_t ext2_ino_by_fname(ext2_t* ext2, const char* filename) {
	char buf[BLOCK_SIZE];
	int32_t depth, i, j, ino;
	str_t* name[MAX_DIR_DEPTH];
	INODE *ip;

	if(strcmp(filename, "/") == 0)
		return 2; //ino 2 for root;
	depth = split_fname(filename, name);

	ino = -1;
	for (j=0; j<ext2->group_num; j++) {
		//if(ext2->read_block((ext2->super.s_blocks_per_group * j) + ext2->gds[j].bg_inode_table+i, buf) == 0) {// read first inode block
		if(ext2->read_block(ext2->gds[j].bg_inode_table, buf) == 0) {
			ip = ((INODE *)buf) + 1;   // ip->root inode #2
			/* serach for system name */
			for (i=0; i<depth; i++) {
				ino = search(ext2, ip, CS(name[i]));
				if (ino < 0) {
					ino = -1;
					break;
				}
				ip = get_node_by_ino(ext2, ino, buf);
				if(ip == NULL) {
					ino = -1;
					break;
				}
			}
		}
		if(ino >= 0)
			break;
	}
	for (i=0; i<depth; i++) {
		str_free(name[i]);
	}
	return ino;
}

static int32_t ext2_node_by_ino(ext2_t* ext2, int32_t ino, INODE* node) {
	char buf[BLOCK_SIZE];
	INODE* p = get_node_by_ino(ext2, ino, buf);
	if(p == NULL)
		return -1;
	memcpy(node, p, sizeof(INODE));
	return 0;
}

static inline int32_t get_gd_num(ext2_t* ext2) {
	int32_t ret = div_u32(ext2->super.s_blocks_count, ext2->super.s_blocks_per_group);
	if(mod_u32(ext2->super.s_blocks_count, ext2->super.s_blocks_per_group) != 0)
		ret++;
	return ret;
}

static int32_t get_gds(ext2_t* ext2) {
	int32_t gd_size = sizeof(GD);
	ext2->group_num = get_gd_num(ext2);
	ext2->gds = (GD*)kmalloc(gd_size * ext2->group_num);

	int32_t gd_num = div_u32(EXT2_BLOCK_SIZE, gd_size);
	int32_t i = 2;
	int32_t index = 0;
	while(true) {
		char buf[EXT2_BLOCK_SIZE];
		ext2->read_block(ext2->super.s_blocks_per_group+i, buf);
		for(int32_t j=0; j<gd_num; j++) {
			memcpy(&ext2->gds[index], buf+(j*gd_size), gd_size);
			index++;
			if(index >= ext2->group_num)
				return 0;
		}
		i++;
	}
	return 0;
}

static int32_t ext2_init(ext2_t* ext2, read_block_func_t read_block, write_block_func_t write_block) {
	if(read_partition() != 0 || partition_get(1, &_partition) != 0) {
		memset(&_partition, 0, sizeof(partition_t));
	}

	char buf[BLOCK_SIZE];
	ext2->read_block = read_block;
	ext2->write_block = write_block;

	ext2->read_block(1, buf);
	memcpy(&ext2->super, buf, sizeof(SUPER));

	get_gds(ext2);

	return 0;
}

static void ext2_quit(ext2_t* ext2) {
	kfree(ext2->gds);
}

static void* ext2_readfile(ext2_t* ext2, const char* fname, int32_t* size) {
	void* ret = NULL;	
	if(size != NULL)
		*size = -1;

	int ino = ext2_ino_by_fname(ext2, fname);
	if(ino >= 0) {
		INODE inode;
		if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
			return ret;
		}

		char *data = (char*)kmalloc(inode.i_size);
		if(data != NULL) {
			ret = data;
			uint32_t rd = 0;
			while(1) {
				int sz = ext2_read(ext2, &inode, data, EXT2_BLOCK_SIZE, rd);
				if(sz <= 0 || rd >= inode.i_size)
					break;
				data += sz;
				rd += sz;
			}
			if(size != NULL)
				*size = rd;
		}
	}
	return ret;
}

void* sd_read_ext2(const char* fname, int32_t* size) {
	ext2_t ext2;
	ext2_init(&ext2, sd_read, NULL);
	void* ret = ext2_readfile(&ext2, fname, size);
	ext2_quit(&ext2);
	return ret;
}

