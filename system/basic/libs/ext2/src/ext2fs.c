#include <sd/sd.h>
#include <ext2/ext2fs.h>
#include <ewoksys/vfs.h>
#include <string.h>
#include <stdlib.h>
#include <ewoksys/mstr.h>

#define SHORT_NAME_MAX 64

/*test bit is on or off*/
static inline int32_t tst_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}

/*set bit off*/
static inline int32_t clr_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] &= ~(1 << j);
	return 0;
}

/*set bit on*/
static inline int32_t set_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] |= (1 << j);
	return 0;
}

/*get group descriptor index by inode*/
static inline uint32_t get_gd_index_by_ino(ext2_t* ext2, uint32_t ino) {
	return ((ino -1) / ext2->super.s_inodes_per_group);
}

/*get group descriptor index by block*/
static inline uint32_t get_gd_index_by_block(ext2_t* ext2, uint32_t block) {
	return ((block-1) / ext2->super.s_blocks_per_group);
}

/*get inode index in group*/
static inline uint32_t get_ino_in_group(ext2_t* ext2, uint32_t ino, uint32_t index) {
	return ino - (index * ext2->super.s_inodes_per_group);
}

/*get block index in group*/
static inline uint32_t get_block_in_group(ext2_t* ext2, uint32_t block, uint32_t index) {
	return block - (index * ext2->super.s_blocks_per_group);
}

/*write back group descriptor by index*/
static int32_t set_gd(ext2_t* ext2, uint32_t index) {
	uint32_t gd_size = sizeof(GD);
	uint32_t gd_num = (EXT2_BLOCK_SIZE / gd_size);

	uint32_t blk_index = (index / gd_num);
	index = blk_index * gd_num;
  
	const char* p = (const char*)&ext2->gds[index];
	return ext2->write_block(blk_index+2, p);
}

/*write back super block*/
static int32_t set_super(ext2_t* ext2) {
	char buf[EXT2_BLOCK_SIZE];
	memcpy(buf, &ext2->super, sizeof(SUPER));
	if(ext2->group_num > 1) {//write backup super block
		if(ext2->write_block(8193, buf) != 0)
			return -1;
	}
	return ext2->write_block(1, buf);
}

static void inc_free_blocks(ext2_t* ext2, uint32_t block) {
	int32_t index = get_gd_index_by_block(ext2, block);
	ext2->gds[index].bg_free_blocks_count++;
	set_gd(ext2, index);

	ext2->super.s_free_blocks_count++;
	set_super(ext2);
}

static void inc_free_inodes(ext2_t* ext2, uint32_t ino) {
	uint32_t index = get_gd_index_by_ino(ext2, ino);
	ext2->gds[index].bg_free_inodes_count++;
	set_gd(ext2, index);

	ext2->super.s_free_inodes_count++;
	set_super(ext2);
}

static void dec_free_blocks(ext2_t* ext2, uint32_t block) {
	uint32_t index = get_gd_index_by_block(ext2, block);
	ext2->gds[index].bg_free_blocks_count--;
	set_gd(ext2, index);

	ext2->super.s_free_blocks_count--;
	set_super(ext2);
}

static void dec_free_inodes(ext2_t* ext2, uint32_t ino) {
	uint32_t index = get_gd_index_by_ino(ext2, ino);
	ext2->gds[index].bg_free_inodes_count--;
	set_gd(ext2, index);

	ext2->super.s_free_inodes_count--;
	set_super(ext2);
}

static int32_t ext2_idealloc(ext2_t* ext2, uint32_t ino) {
	char buf[EXT2_BLOCK_SIZE];
	if (ino > ext2->super.s_inodes_count)
		return -1;

	// get inode bitmap block
	uint32_t index = get_gd_index_by_ino(ext2, ino);
	uint32_t ino_g = get_ino_in_group(ext2, ino, index);

	//uint32_t blk = index*ext2->super.s_blocks_per_group + ext2->gds[index].bg_inode_bitmap;
	uint32_t blk = ext2->gds[index].bg_inode_bitmap;
	if(ext2->read_block(blk, buf) != 0)
		return -1;

	clr_bit(buf, ino_g-1);
	// write buf back
	if(ext2->write_block(blk, buf) != 0) // update free inode count in SUPER and GD
		return -1;
	inc_free_inodes(ext2, ino);
	return 0;
}

static int32_t ext2_bdealloc(ext2_t* ext2, uint32_t block) {
	char buf[EXT2_BLOCK_SIZE];
	if (block == 0)
		return -1;
	memset(buf, 0, EXT2_BLOCK_SIZE);	
	if(ext2->write_block(block, buf) != 0)
		return -1;

	uint32_t index = get_gd_index_by_block(ext2, block);
	uint32_t block_g = get_block_in_group(ext2, block, index);

	//uint32_t blk = index * ext2->super.s_blocks_per_group + ext2->gds[index].bg_block_bitmap;
	uint32_t blk = ext2->gds[index].bg_block_bitmap;
	if(ext2->read_block(blk, buf) != 0)
		return -1;

	clr_bit(buf, block_g-1);
	// write buf back
	if(ext2->write_block(blk, buf) != 0)
		return -1;
	// update free inode count in SUPER and GD
	inc_free_blocks(ext2, block);
	return 0;
}

static uint32_t ext2_ialloc(ext2_t* ext2) {  //alloc a node, inode start with 1 not 0!!
	char buf[EXT2_BLOCK_SIZE];
	uint32_t index = 0;
	uint32_t i, blk = 0, ino = 0;
	for (i=0; i < ext2->super.s_inodes_count; i++){
		ino = i + 1;
		if((i % ext2->super.s_inodes_per_group) == 0) {
			index = get_gd_index_by_ino(ext2, ino);
			//blk = index*ext2->super.s_inodes_per_group + ext2->gds[index].bg_inode_bitmap;
			blk = ext2->gds[index].bg_inode_bitmap;
			if(ext2->read_block(blk, buf) != 0)
				return 0;
		}
	
		uint32_t ino_g = get_ino_in_group(ext2, ino, index);
		if (tst_bit(buf, ino_g-1) == 0){
			set_bit(buf, ino_g-1);
			if(ext2->write_block(blk, buf) != 0)
				return 0;
			// update free inode count in SUPER and GD
			dec_free_inodes(ext2, ino);
			return ino;
		}
	}
	return 0;
} 

static int32_t ext2_balloc(ext2_t* ext2) { //alloc a block
 	char buf[EXT2_BLOCK_SIZE];
	uint32_t index = 0;
	uint32_t i, blk = 0;

	for (i = 0; i < ext2->super.s_blocks_count; i++) {
		uint32_t block = i + 1;
		if((i % ext2->super.s_blocks_per_group) == 0) {
			index = get_gd_index_by_block(ext2, block);
			//blk = index*ext2->super.s_blocks_per_group + ext2->gds[index].bg_block_bitmap;
			blk = ext2->gds[index].bg_block_bitmap;
			if(ext2->read_block(blk, buf) != 0)
				return 0;
		}

		uint32_t block_g = get_block_in_group(ext2, block, index);
		if (tst_bit(buf, block_g-1) == 0) {
			set_bit(buf, block_g-1);
			if(ext2->write_block(blk, buf) != 0)
				return 0;
			dec_free_blocks(ext2, block);
			return block;
		}
	}
	return 0;
}

static int32_t need_len(int32_t len) {
	return 4 * ((8 + len + 3) / 4);
}

static int32_t write_child(ext2_t* ext2, INODE* pip, uint32_t ino, const char *name, int32_t ftype) {
	int32_t nlen, ideal_len, remain, i, blk;
	char buf[EXT2_BLOCK_SIZE];
	char *cp;
	DIR_T *dp = NULL;
	//(1)-(3)
	nlen = need_len(strlen(name));
	for(i=0;i<12;i++){
		//(5) if no block alloced
		if(pip->i_block[i]==0){ 
			blk = ext2_balloc(ext2);
			if(blk==0){
				return -1;
			}
			pip->i_block[i] = blk;
			pip->i_size += EXT2_BLOCK_SIZE;
			//pmip->dirty = 1;
			if(ext2->read_block(pip->i_block[i], buf) != 0)
				return -1;
			memset(buf, 0, EXT2_BLOCK_SIZE);
			dp->inode = ino;
			dp->rec_len = EXT2_BLOCK_SIZE;
			dp->name_len = strlen(name);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, name);
			return ext2->write_block(pip->i_block[i], buf);
		}		

		//if block alloced , initialize it
		if(ext2->read_block(pip->i_block[i], buf) != 0)
			return -1;
		cp = buf;
		dp = (DIR_T *)cp;
		if(dp->inode==0){
			dp->inode = ino;
			dp->rec_len = EXT2_BLOCK_SIZE;
			dp->name_len = strlen(name);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, name);
			return ext2->write_block(pip->i_block[i], buf);
		}

		//(4) get last entry in block
		while ((cp + dp->rec_len) < (buf + EXT2_BLOCK_SIZE)){
			cp += dp->rec_len;
			dp = (DIR_T *)cp;
		}
		ideal_len = need_len(dp->name_len);
		remain = dp->rec_len-ideal_len;
		if(remain >= nlen){
			dp->rec_len = ideal_len;
			cp += dp->rec_len;
			dp = (DIR_T *)cp;
			dp->inode = ino;
			dp->rec_len = remain;
			dp->name_len = strlen(name);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, name);
			return ext2->write_block(pip->i_block[i], buf);
		}
	}
	return -1;
}

static int32_t ext2_rm_child(ext2_t* ext2, INODE *ip, const char *name) {
	int32_t i, rec_len, found, first_len;
	char *cp = NULL, *precp = NULL;
	DIR_T *dp = NULL;
	char buf[EXT2_BLOCK_SIZE];

	found = 0;
	rec_len = 0;
	first_len = 0;
	for(i = 0; i<12; i++) {
		if(ip->i_block[i] == 0) { //cannot find .
			return -1;
		}
		if(ext2->read_block(ip->i_block[i], buf) != 0)
			return -1;
		cp = buf;
		while(cp < (buf + EXT2_BLOCK_SIZE)) {
			dp = (DIR_T *)cp;
			if(found == 0 && dp->inode != 0 && strcmp(dp->name, name) == 0){
				//(2).1. if (first and only entry in a data block){
				if(dp->rec_len == EXT2_BLOCK_SIZE){
					dp->inode = 0;
					return ext2->write_block(ip->i_block[i], buf);
				}
				//(2).2. else if LAST entry in block
				else if(precp != NULL && (dp->rec_len + (cp - buf)) == EXT2_BLOCK_SIZE){
					dp = (DIR_T *)precp;
					dp->rec_len = EXT2_BLOCK_SIZE - (precp - buf);
					return ext2->write_block(ip->i_block[i], buf);
				}
				//(2).3. else: entry is first but not the only entry or in the middle of a block:
				else{
					found = 1;
					rec_len = dp->rec_len;
					first_len = cp-buf;
				}
			}
			if(found == 0)
				precp = cp;
			cp += dp->rec_len;
		}

		if(found == 1) {
			dp->rec_len += rec_len;			
			char cpbuf[EXT2_BLOCK_SIZE];
			memset(cpbuf, 0, EXT2_BLOCK_SIZE);
			memcpy(cpbuf, buf, first_len);
			memcpy(cpbuf + first_len, buf + first_len + rec_len, EXT2_BLOCK_SIZE-(first_len+rec_len));
			return ext2->write_block(ip->i_block[i], cpbuf);
		}
	}
	return -1;
}

int32_t ext2_read_block(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset) {
	//(2) count = 0
	// avil = fileSize - OFT's offset // number of bytes still available in file.
	int32_t count_read = 0;
	char *cq = buf;
	int32_t avil = node->i_size - offset;
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	//(3)
	/*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
		lbk       = oftp->offset / EXT2_BLOCK_SIZE;
		start_byte = oftp->offset % EXT2_BLOCK_SIZE;*/	
	lbk = offset / EXT2_BLOCK_SIZE;
	start_byte = offset % EXT2_BLOCK_SIZE;
	if(nbytes > (EXT2_BLOCK_SIZE - start_byte))
		nbytes = (EXT2_BLOCK_SIZE - start_byte);
	//(5) READ
	//(5).1 direct blocks
	if(lbk < 12) {
		blk = node->i_block[lbk];
	}
	//(5).2 Indirect blocks contains 256 block number 
	else if(lbk>=12 && lbk < 256 +12){
		int32_t indirect_buf[256];
		if(ext2->read_block(node->i_block[12], (char*)indirect_buf) != 0)
			return -1;
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
		if(ext2->read_block(node->i_block[13], (char*)double_buf1) != 0)
			return -1;
		int32_t double_buf2[256];
		if(ext2->read_block(double_buf1[num], (char*)double_buf2) != 0)
			return -1;
		blk = double_buf2[pos_offset];
	}

	char readbuf[EXT2_BLOCK_SIZE];
	if(ext2->read_block(blk, readbuf) != 0)
		return -1;
	char *cp = readbuf + start_byte;
	remain = EXT2_BLOCK_SIZE - start_byte;
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

int32_t ext2_read(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset) {
	char* p = buf;
	int32_t ret = nbytes;
	while(nbytes > 0) {
		int32_t rd = ext2_read_block(ext2, node, p, nbytes, offset);
		if(rd <= 0)
			return 0;
		nbytes -= rd;
		offset += rd;
		p += rd;
	}
	return ret;
}

static INODE* get_node_by_ino(ext2_t* ext2, uint32_t ino, char* buf) {
	uint32_t bgid = get_gd_index_by_ino(ext2, ino);
	ino = get_ino_in_group(ext2, ino, bgid);
	uint32_t offset = (ino-1)%8;

	//uint32_t blk = bgid*ext2->super.s_blocks_per_group + ext2->gds[bgid].bg_inode_table + 	((ino-1)/8);
	uint32_t blk = ext2->gds[bgid].bg_inode_table + 	((ino-1)/8);
	if(ext2->read_block(blk, buf) != 0)
		return NULL;
	return ((INODE *)buf) + offset;
}

int32_t put_node(ext2_t* ext2, uint32_t ino, INODE *node) {
	uint32_t bgid = get_gd_index_by_ino(ext2, ino);
	ino = get_ino_in_group(ext2, ino, bgid);
	uint32_t offset = (ino-1)%8;

	//uint32_t blk = bgid*ext2->super.s_inodes_per_group + ext2->gds[bgid].bg_inode_table + 	((ino-1)/8);
	uint32_t blk = ext2->gds[bgid].bg_inode_table + (ino-1)/8;
	char buf[EXT2_BLOCK_SIZE];
	if(ext2->read_block(blk, buf) != 0)
		return -1;
	INODE *ip = ((INODE *)buf) + offset;
	memcpy(ip, node, sizeof(INODE));
	return ext2->write_block(blk, buf);	
}

int32_t ext2_create_dir(ext2_t* ext2, INODE* father_inp, const char *name,
		uint16_t uid, uint16_t gid, uint16_t mode ) {
	uint32_t ino, i, blk;
	char buf[EXT2_BLOCK_SIZE];

	ino = ext2_ialloc(ext2); //alloc a node id from table
	blk = ext2_balloc(ext2); //alloc a block

	INODE* inp = get_node_by_ino(ext2, ino, buf); //read inode from block
	if(inp == NULL)
		return -1;
	//set inode info
	inp->i_mode = mode;
	inp->i_uid  = uid;
	inp->i_gid  = gid;
	inp->i_size = EXT2_BLOCK_SIZE;	        // Size in bytes (one block for dir)
	inp->i_links_count = 0;	  //
	inp->i_atime = 0;         // TODO Set last access to current time
	inp->i_ctime = 0;         // TODO  Set creation to current time
	inp->i_mtime = 0;         // TODO Set last modified to current time
	inp->i_blocks = EXT2_BLOCK_SIZE / 512; // # of 512-byte blocks reserved for this inode 
	inp->i_block[0] = blk;
	for(i=1; i<15; i++){
		inp->i_block[i] = 0;
	}
	//mip->dirty = 1;

	if(put_node(ext2, ino, inp) != 0)
		return -1; //write inode back to block

	if(write_child(ext2, father_inp, ino, name, EXT2_FT_DIR) < 0) //write dir info (name, type)
		return -1;
	return ino;
}

int32_t ext2_create_file(ext2_t* ext2, INODE* father_inp, const char *name,
		uint16_t uid, uint16_t gid, uint16_t mode ) {
	uint32_t ino, i, blk;
	char buf[EXT2_BLOCK_SIZE];

	ino = ext2_ialloc(ext2);
	if(ino == 0)
		return -1;

	INODE* inp = get_node_by_ino(ext2, ino, buf);
	if(inp == NULL)
		return -1;
	inp->i_mode = mode;
	inp->i_uid  = uid;
	inp->i_gid  = gid;
	inp->i_size = 0;	        // Size in bytes 
	inp->i_links_count = 0;	  // . and ..
	inp->i_atime = 0;         // TODO Set last access to current time
	inp->i_ctime = 0;         // TODO  Set creation to current time
	inp->i_mtime = 0;         // TODO Set last modified to current time
	inp->i_blocks = 0;        // # of 512-byte blocks reserved for this inode
	for(i=0; i<15; i++){
		inp->i_block[i] = 0;
	}
	//mip->dirty = 1;
	if(put_node(ext2, ino, inp) != 0)
		return -1;

	if(write_child(ext2, father_inp, ino, name, EXT2_FT_FILE) < 0)
		return -1;
	return ino;
}

int32_t ext2_write(ext2_t* ext2, INODE* node, const char *data, int32_t nbytes, int32_t offset) {
	static char buf[EXT2_BLOCK_SIZE];
	const char *cq = data;
	char *cp;
	//(2)
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	int32_t nbytes_copy = 0;
	//(3)
	/*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
		lbk       = oftp->offset / EXT2_BLOCK_SIZE;
		start_byte = oftp->offset % EXT2_BLOCK_SIZE;*/	
	lbk = (offset / EXT2_BLOCK_SIZE);
	start_byte = offset % EXT2_BLOCK_SIZE;
	//5.1 direct
	if(lbk < 12){
		//if its the first one of the new block, allocatea new one
		if(node->i_block[lbk] == 0){
			node->i_block[lbk] = ext2_balloc(ext2);
		}
		blk = node->i_block[lbk];
	}
	//5.2 indirect 
	else if(lbk >=12 && lbk < 256+12){
		int32_t* indirect_buf = (int32_t*)buf;
		//if its the first one of the new block, allocatea new one
		if(node->i_block[12] == 0){
			node->i_block[12] = ext2_balloc(ext2);
			memset(indirect_buf, 0, EXT2_BLOCK_SIZE);
			if(ext2->write_block(node->i_block[12], (char*)indirect_buf) != 0)
				return 0;
		}
		if(ext2->read_block(node->i_block[12], (char*)indirect_buf) != 0)
			return 0;
		if(indirect_buf[lbk-12] == 0){
			indirect_buf[lbk-12] = ext2_balloc(ext2);
			if(ext2->write_block(node->i_block[12], (char*)indirect_buf) != 0)
				return 0;
		}
		blk = indirect_buf[lbk-12]; 
	}
	else{
		return nbytes_copy;
	}

	if(ext2->read_block(blk, buf) != 0)
		return 0;
	cp = buf + start_byte;
	remain = EXT2_BLOCK_SIZE - start_byte;
	while(remain > 0){
		int32_t min = 0;
		if(nbytes<=remain){
			min = nbytes;
		}
		else{
			min = remain;
		}
		memcpy(cp, cq, nbytes);
		nbytes_copy += min;
		nbytes -= min;
		remain -= min;
		offset += min;
		if(offset > (int32_t)node->i_size){
			node->i_size += min;
		}			
		if(nbytes<=0){
			break;
		}
	}
	if(ext2->write_block(blk, buf) != 0)
		return 0;
	return nbytes_copy;
}

static uint32_t search(ext2_t* ext2, INODE *ip, const char *name) {
	int32_t i; 
	char c, *cp;
	DIR_T  *dp;
	char buf[EXT2_BLOCK_SIZE];

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			if(ext2->read_block(ip->i_block[i], buf) != 0)
				return 0;
			dp = (DIR_T *)buf;
			cp = buf;
			while (cp < &buf[EXT2_BLOCK_SIZE]){
				if(dp->name_len == 0 || dp->rec_len == 0)
					break;
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if (strcmp(dp->name, name) == 0 ){
					return dp->inode;
				}
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR_T *)cp;
			}
		}
	}
	return 0;
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

uint32_t ext2_ino_by_fname(ext2_t* ext2, const char* filename) {
	char buf[EXT2_BLOCK_SIZE];
	uint32_t depth, i, ino, blk;
	str_t* name[MAX_DIR_DEPTH];
	INODE *ip;

	if(strcmp(filename, "/") == 0)
		return 2; //ino 2 for root;
	depth = split_fname(filename, name);

	ino = 0;
	for (int32_t j=0; j<ext2->group_num; j++) {
		//blk = j*ext2->super.s_inodes_per_group + ext2->gds[j].bg_inode_table;
		blk = ext2->gds[j].bg_inode_table;
		if(ext2->read_block(blk, buf) == 0) {// read first inode block
			ip = ((INODE *)buf) + 1;   // ip->root inode #2
			/* serach for system name */
			for (i=0; i<depth; i++) {
				ino = search(ext2, ip, CS(name[i]));
				if (ino == 0) {
					break;
				}
				ip = get_node_by_ino(ext2, ino, buf);
				if(ip == NULL) {
					ino = 0;
					break;
				}
			}
		}
		if(ino > 0)
			break;
	}
	for (i=0; i<depth; i++) {
		str_free(name[i]);
	}
	return ino;
}

int32_t ext2_node_by_fname(ext2_t* ext2, const char* filename, INODE* inode) {
	uint32_t ino = ext2_ino_by_fname(ext2, filename);
	if(ino == 0)
		return -1;
	return ext2_node_by_ino(ext2, ino, inode);
}

//TODO
int32_t ext2_unlink(ext2_t* ext2, const char* fname) {
	char buf[EXT2_BLOCK_SIZE];
	char dir[FS_FULL_NAME_MAX];
	char name[FS_FULL_NAME_MAX];
	vfs_dir_name(fname, dir, FS_FULL_NAME_MAX);
	vfs_file_name(fname, name, FS_FULL_NAME_MAX);

	uint32_t fino = ext2_ino_by_fname(ext2, dir);
	if(fino == 0) {
		return -1;
	}

	INODE* fnode = get_node_by_ino(ext2, fino, buf);
	if(fnode == NULL) {
		return -1;
	}

	uint32_t ino = search(ext2, fnode, name);
	if(ino == 0) {
		return -1;
	}

	ext2_rm_child(ext2, fnode, name);
	if(put_node(ext2, fino, fnode) != 0)
		return -1;

	INODE* node = get_node_by_ino(ext2, ino, buf);
	if(node == NULL) {
		return -1;
	}

	//(3) -(5)
	if(node->i_links_count > 0){
		//node->dirty = 1;
	}
	//if(!S_ISLNK(ip->i_mode)){
	int32_t i;
	for(i = 0; i < 12; i++) {
		if(node->i_block[i] == 0)
			break;
		ext2_bdealloc(ext2, node->i_block[i]);
		node->i_block[i] = 0;
	}
	//}
	//node->dirty = 1;
	if(put_node(ext2, ino, node) != 0)
		return -1;
	ext2_idealloc(ext2, ino);
	return 0;
}

int32_t ext2_node_by_ino(ext2_t* ext2, uint32_t ino, INODE* node) {
	char buf[EXT2_BLOCK_SIZE];
	INODE* p = get_node_by_ino(ext2, ino, buf);
	if(p == NULL)
		return -1;
	memcpy(node, p, sizeof(INODE));
	return 0;
}

int32_t ext2_rmdir(ext2_t* ext2, const char *fname) {
	return ext2_unlink(ext2, fname);
}

static inline int32_t get_gd_num(ext2_t* ext2) {
	int32_t ret = (ext2->super.s_blocks_count / ext2->super.s_blocks_per_group);
	if((ext2->super.s_blocks_count % ext2->super.s_blocks_per_group) != 0)
		ret++;
	return ret;
}

static int32_t get_gds(ext2_t* ext2) {
  int32_t gd_size = sizeof(GD);
  ext2->group_num = get_gd_num(ext2);
  ext2->gds = (GD*)malloc(gd_size * ext2->group_num);

  int32_t gd_num = (EXT2_BLOCK_SIZE / gd_size);
  int32_t i = 2;
  int32_t index = 0;
  while(true) {
    char buf[EXT2_BLOCK_SIZE];
    if(ext2->read_block(i, buf) != 0)
			return 0;
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

int32_t ext2_init(ext2_t* ext2, read_block_func_t read_block, write_block_func_t write_block, uint32_t buffer_size) {
	char buf[EXT2_BLOCK_SIZE];
	ext2->read_block = read_block;
	ext2->write_block = write_block;

	//read super block
	if(ext2->read_block(1, buf) != 0)
		return -1;
	memcpy(&ext2->super, buf, sizeof(SUPER));
	get_gds(ext2);
	sd_set_max_sector_index(ext2->super.s_blocks_count);
	sd_set_buffer_size(buffer_size);
	return 0;
}

void ext2_quit(ext2_t* ext2) {
	free(ext2->gds);
}

void* ext2_readfile(ext2_t* ext2, const char* fname, int32_t* size) {
  void* ret = NULL;
  if(size != NULL)
    *size = -1;

  uint32_t ino = ext2_ino_by_fname(ext2, fname);
  if(ino > 0) {
    INODE inode;
    if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
      return ret;
    }

    char *data = (char*)malloc(inode.i_size+1);
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

