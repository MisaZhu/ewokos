#include <ext2fs.h>
#include <sys/vfs.h>
#include <string.h>
#include <stdlib.h>
#include <mstr.h>

#define SHORT_NAME_MAX 64
#define EXT2_BLOCK_SIZE 1024

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
static inline int32_t get_gd_index_by_ino(ext2_t* ext2, int32_t ino) {
	return div_u32(ino, ext2->super.s_inodes_per_group);
}

/*get group descriptor index by block*/
static inline int32_t get_gd_index_by_block(ext2_t* ext2, int32_t block) {
	return div_u32(block, ext2->super.s_blocks_per_group);
}

/*get inode index in group*/
static inline int32_t get_ino_in_group(ext2_t* ext2, int32_t ino, int32_t index) {
	return ino - (index * ext2->super.s_inodes_per_group);
}

/*get block index in group*/
static inline int32_t get_block_in_group(ext2_t* ext2, int32_t block, int32_t index) {
	return block - (index * ext2->super.s_blocks_per_group);
}

/*write back group descriptor by index*/
static int32_t set_gd(ext2_t* ext2, int32_t index) {
	char buf[BLOCK_SIZE];
	memcpy(buf, &ext2->gds[index], sizeof(GD));
	return ext2->write_block(index*ext2->super.s_blocks_per_group+2, buf);
}

/*write back super block*/
static int32_t set_super(ext2_t* ext2) {
	char buf[BLOCK_SIZE];
	memcpy(buf, &ext2->super, sizeof(SUPER));
	if(ext2->group_num > 1) //write backup super block
		ext2->write_block(8193, buf);
	return ext2->write_block(1, buf);
}

/*
static void inc_free_blocks(ext2_t* ext2, int32_t block) {
	ext2->super.s_free_blocks_count++;
	set_super(ext2);

	int32_t index = get_gd_index_by_block(ext2, block);
	ext2->gds[index].bg_free_blocks_count++;
	set_gd(ext2, index);
}
*/

static void inc_free_inodes(ext2_t* ext2, int32_t ino) {
	int32_t index = get_gd_index_by_ino(ext2, ino);
	ext2->gds[index].bg_free_inodes_count++;
	set_gd(ext2, index);

	ext2->super.s_free_inodes_count++;
	set_super(ext2);
}

static void dec_free_blocks(ext2_t* ext2, int32_t block) {
	int32_t index = get_gd_index_by_block(ext2, block);
	ext2->gds[index].bg_free_blocks_count--;
	set_gd(ext2, index);

	ext2->super.s_free_blocks_count--;
	set_super(ext2);
}

static void dec_free_inodes(ext2_t* ext2, int32_t ino) {
	int32_t index = get_gd_index_by_ino(ext2, ino);
	ext2->gds[index].bg_free_inodes_count--;
	set_gd(ext2, index);

	ext2->super.s_free_inodes_count--;
	set_super(ext2);
}

static void ext2_idealloc(ext2_t* ext2, int32_t ino) {
	char buf[BLOCK_SIZE];
	if (ino > (int32_t)ext2->super.s_inodes_count)
		return;

	// get inode bitmap block
	int32_t index = get_gd_index_by_ino(ext2, ino);
	if(ext2->read_block(ext2->gds[index].bg_inode_bitmap, buf) != 0)
		return;

	clr_bit(buf, ino-1);
	// write buf back
	if(ext2->write_block(ext2->gds[index].bg_inode_bitmap, buf) == 0) // update free inode count in SUPER and GD
		inc_free_inodes(ext2, ino);
}

static int32_t ext2_bdealloc(ext2_t* ext2, int32_t block) {
	char buf[BLOCK_SIZE];
	if (block <= 0)
		return -1;

	int32_t index = get_gd_index_by_block(ext2, block);
	if(ext2->read_block(ext2->gds[index].bg_block_bitmap, buf) != 0)
		return -1;
	clr_bit(buf, block-1);
	// write buf back
	if(ext2->write_block(ext2->gds[index].bg_block_bitmap, buf) != 0)
		return -1;
	// update free inode count in SUPER and GD
	dec_free_blocks(ext2, block);
	return 0;
}

static uint32_t ext2_ialloc(ext2_t* ext2) {
	char buf[BLOCK_SIZE];
	int32_t index = 0;
	uint32_t i;
	for (i=0; i < ext2->super.s_inodes_count; i++){
		if(mod_u32(i, ext2->super.s_inodes_per_group) == 0) {
			index = get_gd_index_by_ino(ext2, i);
			ext2->read_block(ext2->gds[index].bg_inode_bitmap, buf);
		}
	
		uint32_t ino = get_ino_in_group(ext2, i, index);
		if (tst_bit(buf, ino) == 0){
			set_bit(buf, ino);
			ext2->write_block(ext2->gds[index].bg_inode_bitmap, buf);
			// update free inode count in SUPER and GD
			dec_free_inodes(ext2, i);
			return (i+1);
		}
	}
	return 0;
} 

static int32_t ext2_balloc(ext2_t* ext2) {
 	char buf[BLOCK_SIZE];
	int32_t index = 0;
	uint32_t i;

	ext2->read_block(ext2->gds[0].bg_block_bitmap, buf);
	for (i = 0; i < ext2->super.s_blocks_count; i++) {
		if(mod_u32(i, ext2->super.s_blocks_per_group) == 0) {
			index = get_gd_index_by_block(ext2, i);
			ext2->read_block(ext2->gds[index].bg_block_bitmap, buf);
		}

		uint32_t block = get_block_in_group(ext2, i, index);
		if (tst_bit(buf, block) == 0) {
			set_bit(buf, block);
			dec_free_blocks(ext2, i);
			ext2->write_block(ext2->gds[index].bg_block_bitmap, buf);
			return i+1;
		}
	}
	return 0;
}

static int32_t need_len(int32_t len) {
	return 4 * ((8 + len + 3) / 4);
}

static int32_t enter_child(ext2_t* ext2, INODE* pip, int32_t ino, const char *base, int32_t ftype) {
	int32_t nlen, ideal_len, remain, i, blk;
	char buf[BLOCK_SIZE];
	char *cp;
	DIR *dp = NULL;
	//(1)-(3)
	nlen = need_len(strlen(base));
	for(i=0;i<12;i++){
		//(5) first one creat 
		if(dp != NULL && pip->i_block[i]==0){
			blk = ext2_balloc(ext2);
			if(blk<=0){
				return -1;
			}
			pip->i_block[i] = blk;
			pip->i_size += BLOCK_SIZE;
			//pmip->dirty = 1;
			ext2->read_block(pip->i_block[i], buf);
			dp->inode = ino;
			dp->rec_len = BLOCK_SIZE;
			dp->name_len = strlen(base);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, base);
			ext2->write_block(pip->i_block[i], buf);
			return 0;
		}		
		//initialize the created one
		ext2->read_block(pip->i_block[i], buf);
		cp = buf;
		dp = (DIR *)cp;
		if(dp->inode==0){
			dp->inode = ino;
			dp->rec_len = BLOCK_SIZE;
			dp->name_len = strlen(base);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, base);
			ext2->write_block(pip->i_block[i], buf);
			return 0;
		}

		//(4) get last entry in block
		while (cp + dp->rec_len < buf + BLOCK_SIZE){
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		ideal_len = need_len(dp->name_len);
		remain = dp->rec_len-ideal_len;
		if(remain >= nlen){
			dp->rec_len = ideal_len;
			cp += dp->rec_len;
			dp = (DIR *)cp;
			dp->inode = ino;
			dp->rec_len = remain;
			dp->name_len = strlen(base);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, base);
			ext2->write_block(pip->i_block[i], buf);
			return 0;
		}
	}
	return -1;
}

static int32_t ext2_rm_child(ext2_t* ext2, INODE *ip, const char *name) {
	int32_t i, rec_len, found, first_len;
	char *cp = NULL, *precp = NULL;
	DIR *dp = NULL;
	char buf[BLOCK_SIZE];

	found = 0;
	rec_len = 0;
	first_len = 0;
	for(i = 0; i<12; i++) {
		if(ip->i_block[i] == 0) { //cannot find .
			return -1;
		}
		ext2->read_block(ip->i_block[i], buf);
		cp = buf;
		while(cp < buf + BLOCK_SIZE) {
			dp = (DIR *)cp;
			if(found == 0 && dp->inode!=0 && strcmp(dp->name, name)==0){
				//(2).1. if (first and only entry in a data block){
				if(dp->rec_len == BLOCK_SIZE){
					dp->inode = 0;
					ext2->write_block(ip->i_block[i], buf);
					return 0;
				}
				//(2).2. else if LAST entry in block
				else if(precp != NULL && dp->rec_len + (cp - buf) == BLOCK_SIZE){
					dp = (DIR *)precp;
					dp->rec_len = BLOCK_SIZE - (precp - buf);
					ext2->write_block(ip->i_block[i], buf);
					return 0;
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
			char cpbuf[BLOCK_SIZE];
			memcpy(cpbuf, buf, first_len);
			memcpy(cpbuf + first_len, buf + first_len + dp->rec_len, BLOCK_SIZE-(first_len+rec_len));
			ext2->write_block(ip->i_block[i], cpbuf);
			return 0;
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
		if(ext2->read_block(node->i_block[13], (char*)double_buf1) != 0)
			return -1;
		int32_t double_buf2[256];
		if(ext2->read_block(double_buf1[num], (char*)double_buf2) != 0)
			return -1;
		blk = double_buf2[pos_offset];
	}

	char readbuf[BLOCK_SIZE];
	if(ext2->read_block(blk, readbuf) != 0)
		return -1;
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

static INODE* get_node_by_ino(ext2_t* ext2, int32_t ino, char* buf) {
	int32_t bgid = get_gd_index_by_ino(ext2, ino);
	ino = get_ino_in_group(ext2, ino, bgid);
	int32_t offset = (ino-1)%8;

	ext2->read_block(ext2->gds[bgid].bg_inode_table + 	((ino-1)/8), buf);
	return ((INODE *)buf) + offset;
}

void put_node(ext2_t* ext2, int32_t ino, INODE *node) {
	int32_t bgid = get_gd_index_by_ino(ext2, ino);
	ino = get_ino_in_group(ext2, ino, bgid);
	int32_t offset = (ino-1)%8;

	int32_t blk = bgid*ext2->super.s_blocks_per_group + ext2->gds[bgid].bg_inode_table + 	((ino-1)/8);
	char buf[BLOCK_SIZE];
	ext2->read_block(blk, buf);
	INODE *ip = ((INODE *)buf) + offset;
	*ip = *node;
	ext2->write_block(blk, buf);	
}

int32_t ext2_create_dir(ext2_t* ext2, INODE* father_inp, const char *base, int32_t owner) { //mode file or dir
	int32_t ino, i, blk;
	char buf[BLOCK_SIZE];

	ino = ext2_ialloc(ext2);
	blk = ext2_balloc(ext2);

	INODE* inp = get_node_by_ino(ext2, ino, buf);
	inp->i_mode = 0;//TODO EXT2_DIR_MODE;
	inp->i_uid  = owner & 0xffff;
	inp->i_gid  = (owner >> 16) & 0xffff;
	inp->i_size = BLOCK_SIZE;	        // Size in bytes (one block for dir)
	inp->i_links_count = 0;	  //
	inp->i_atime = 0;         // TODO Set last access to current time
	inp->i_ctime = 0;         // TODO  Set creation to current time
	inp->i_mtime = 0;         // TODO Set last modified to current time
	inp->i_blocks = BLOCK_SIZE / 512; // # of 512-byte blocks reserved for this inode 
  inp->i_block[0] = blk;
	for(i=1; i<15; i++){
		inp->i_block[i] = 0;
	}
	//mip->dirty = 1;
	put_node(ext2, ino, inp);
	if(enter_child(ext2, father_inp, ino, base, EXT2_FT_DIR) < 0)
		return -1;
	return ino;
}

int32_t ext2_create_file(ext2_t* ext2, INODE* father_inp, const char *base, int32_t owner) { //mode file or dir
	int32_t ino, i;
	char buf[BLOCK_SIZE];

	ino = ext2_ialloc(ext2);

	INODE* inp = get_node_by_ino(ext2, ino, buf);
	inp->i_mode = 0; //TODO EXT2_FILE_MODE;
	inp->i_uid  = owner & 0xffff;
	inp->i_gid  = (owner >> 16) & 0xffff;
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
	put_node(ext2, ino, inp);

	if(enter_child(ext2, father_inp, ino, base, EXT2_FT_FILE) < 0)
		return -1;
	return ino;
}

int32_t ext2_write(ext2_t* ext2, INODE* node, const char *data, int32_t nbytes, int32_t offset) {
	static char buf[BLOCK_SIZE];
	const char *cq = data;
	char *cp;
	//(2)
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	int32_t nbytes_copy = 0;
	//(3)
	/*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
		lbk       = oftp->offset / BLOCK_SIZE;
		start_byte = oftp->offset % BLOCK_SIZE;*/	
	lbk = offset / BLOCK_SIZE;
	start_byte = offset % BLOCK_SIZE;
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
			memset(indirect_buf, 0, BLOCK_SIZE);
			ext2->write_block(node->i_block[12], (char*)indirect_buf);
		}
		ext2->read_block(node->i_block[12], (char*)indirect_buf);
		if(indirect_buf[lbk-12] == 0){
			indirect_buf[lbk-12] = ext2_balloc(ext2);
			ext2->write_block(node->i_block[12], (char*)indirect_buf);
		}
		blk = indirect_buf[lbk-12]; 
	}
	else{
		return nbytes_copy;
	}

	ext2->read_block(blk, buf);
	cp = buf + start_byte;
	remain = BLOCK_SIZE - start_byte;
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
	ext2->write_block(blk, buf);
	return nbytes_copy;
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

int32_t ext2_ino_by_fname(ext2_t* ext2, const char* filename) {
	char buf[BLOCK_SIZE];
	int32_t depth, i, ino;
	str_t* name[MAX_DIR_DEPTH];
	INODE *ip;

	if(strcmp(filename, "/") == 0)
		return 2; //ino 2 for root;
	depth = split_fname(filename, name);

	ino = -1;
	for (int32_t j=0; j<ext2->group_num; j++) {
		if(ext2->read_block(ext2->gds[j].bg_inode_table, buf) == 0) {// read first inode block
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

int32_t ext2_node_by_fname(ext2_t* ext2, const char* filename, INODE* inode) {
	int32_t ino = ext2_ino_by_fname(ext2, filename);
	if(ino <= 0)
		return -1;
	return ext2_node_by_ino(ext2, ino, inode);
}

//TODO
int32_t ext2_unlink(ext2_t* ext2, const char* fname) {
	char buf[BLOCK_SIZE];
	str_t* dir = str_new("");
	str_t* name = str_new("");
	vfs_parse_name(fname, dir, name);

	int32_t fino = ext2_ino_by_fname(ext2, CS(dir));
	if(fino < 0) {
		str_free(dir);
		str_free(name);
		return -1;
	}
	INODE* fnode = get_node_by_ino(ext2, fino, buf);
	str_free(dir);
	if(fnode == NULL) {
		str_free(name);
		return -1;
	}
	int32_t ino = search(ext2, fnode, CS(name));
	ext2_rm_child(ext2, fnode, CS(name));
	str_free(name);
	put_node(ext2, fino, fnode);

	if(ino < 0)
		return -1;
	INODE* node = get_node_by_ino(ext2, ino, buf);
	if(node == NULL) 
		return -1;

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
	}
	//}
	ext2_idealloc(ext2, ino);
	//node->dirty = 1;
	put_node(ext2, ino, node);	
	return 0;
}

int32_t ext2_node_by_ino(ext2_t* ext2, int32_t ino, INODE* node) {
	char buf[BLOCK_SIZE];
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
	int32_t ret = div_u32(ext2->super.s_blocks_count, ext2->super.s_blocks_per_group);
	if(mod_u32(ext2->super.s_blocks_count, ext2->super.s_blocks_per_group) != 0)
		ret++;
	return ret;
}

static int32_t get_gds(ext2_t* ext2) {
  int32_t gd_size = sizeof(GD);
  ext2->group_num = get_gd_num(ext2);
  ext2->gds = (GD*)malloc(gd_size * ext2->group_num);

  int32_t gd_num = EXT2_BLOCK_SIZE/gd_size;
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

int32_t ext2_init(ext2_t* ext2, read_block_func_t read_block, write_block_func_t write_block) {
	char buf[BLOCK_SIZE];
	ext2->read_block = read_block;
	ext2->write_block = write_block;

	//read super block
	ext2->read_block(1, buf);
	memcpy(&ext2->super, buf, sizeof(SUPER));

	get_gds(ext2);
	return 0;
}

void ext2_quit(ext2_t* ext2) {
	free(ext2->gds);
}

void* ext2_readfile(ext2_t* ext2, const char* fname, int32_t* size) {
  void* ret = NULL;
  if(size != NULL)
    *size = -1;

  int ino = ext2_ino_by_fname(ext2, fname);
  if(ino >= 0) {
    INODE inode;
    if(ext2_node_by_ino(ext2, ino, &inode) != 0) {
      return ret;
    }

    char *data = (char*)malloc(inode.i_size);
    if(data != NULL) {
      ret = data;
      uint32_t rd = 0;
      while(1) {
        int sz = ext2_read(ext2, &inode, data, BLOCK_SIZE, rd);
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

