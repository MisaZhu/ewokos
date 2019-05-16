#include <ext2fs.h>
#include <vfs/fs.h>
#include <kstring.h>
#include <stdlib.h>
#include <tstr.h>

static int32_t tst_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}

static int32_t clr_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] &= ~(1 << j);
	return 0;
}


static int32_t set_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] |= (1 << j);
	return 0;
}

/*static void inc_free_blocks(ext2_t* ext2) {
	char buf[SDC_BLOCK_SIZE];
	// inc free block count in SUPER and GD
	ext2->read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	ext2->write_block(1, buf);

	ext2->read_block(2, buf);
	GD* gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	ext2->write_block(2, buf);
}
*/

static void inc_free_inodes(ext2_t* ext2) {
	char buf[SDC_BLOCK_SIZE];
	ext2->read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	ext2->write_block(1, buf);

	ext2->read_block(2, buf);
	GD* gp = (GD *)buf;
	gp->bg_free_inodes_count++;
	ext2->write_block(2, buf);  
}

static void dec_free_blocks(ext2_t* ext2) {
	char buf[SDC_BLOCK_SIZE];
	// inc free block count in SUPER and GD
	ext2->read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	ext2->write_block(1, buf);

	ext2->read_block(2, buf);
	GD* gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	ext2->write_block(2, buf);
}

static void dec_free_inodes(ext2_t* ext2) {
	char buf[SDC_BLOCK_SIZE];
	ext2->read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	ext2->write_block(1, buf);

	ext2->read_block(2, buf);
	GD* gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	ext2->write_block(2, buf);  
}

static void idealloc(ext2_t* ext2, int32_t ino) {
	char buf[SDC_BLOCK_SIZE];

	if (ino > ext2->ninodes){
		return;
	}
	// get inode bitmap block
	ext2->read_block(ext2->imap, buf);
	clr_bit(buf, ino-1);
	// write buf back
	ext2->write_block(ext2->imap, buf);
	// update free inode count in SUPER and GD
	inc_free_inodes(ext2);
}

int32_t bdealloc(ext2_t* ext2, int32_t bit) {
	char buf[SDC_BLOCK_SIZE];

	if (bit <= 0){
		return -1;
	}

	ext2->read_block(ext2->bmap, buf);
	clr_bit(buf, bit-1);

	// write buf back
	ext2->write_block(ext2->bmap, buf);
	// update free inode count in SUPER and GD
	dec_free_blocks(ext2);
	return 0;
}

static uint32_t ext2_ialloc(ext2_t* ext2) {
	char buf[SDC_BLOCK_SIZE];
	// get inode Bitmap into buf
	ext2->read_block(ext2->imap, buf);
	int32_t i;
	for (i=0; i < ext2->ninodes; i++){
		if (tst_bit(buf, i) == 0){
			set_bit(buf, i);
			ext2->write_block(ext2->imap, buf);
			// update free inode count in SUPER and GD
			dec_free_inodes(ext2);
			return (i+1);
		}
	}
	return 0;
} 

static int32_t ext2_balloc(ext2_t* ext2) {
 	char buf[SDC_BLOCK_SIZE];
	ext2->read_block(ext2->bmap, buf);

	int32_t i;
	for (i = 0; i < ext2->nblocks; i++) {
		if (tst_bit(buf, i) == 0) {
			set_bit(buf, i);
			dec_free_blocks(ext2);
			ext2->write_block(ext2->bmap, buf);
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
	char sbuf[SDC_BLOCK_SIZE], *cp;
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
			pip->i_size += SDC_BLOCK_SIZE;
			//pmip->dirty = 1;
			ext2->read_block(pip->i_block[i], sbuf);
			dp->inode = ino;
			dp->rec_len = SDC_BLOCK_SIZE;
			dp->name_len = strlen(base);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, base);
			ext2->write_block(pip->i_block[i], sbuf);
			return 0;
		}		
		//initialize the created one
		ext2->read_block(pip->i_block[i], sbuf);
		cp = sbuf;
		dp = (DIR *)cp;
		if(dp->inode==0){
			dp->inode = ino;
			dp->rec_len = SDC_BLOCK_SIZE;
			dp->name_len = strlen(base);
			dp->file_type = (uint8_t)ftype;
			strcpy(dp->name, base);
			ext2->write_block(pip->i_block[i], sbuf);
			return 0;
		}

		//(4) get last entry in block
		while (cp + dp->rec_len < sbuf + SDC_BLOCK_SIZE){
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
			ext2->write_block(pip->i_block[i], sbuf);
			return 0;
		}
	}
	return -1;
}

static int32_t ext2_rm_child(ext2_t* ext2, INODE *ip, const char *name) {
	int32_t i, rec_len, found, first_len;
	char *cp = NULL, *precp = NULL;
	DIR *dp = NULL;

	char sbuf[SDC_BLOCK_SIZE];
	char cpbuf[SDC_BLOCK_SIZE];

	found = 0;
	rec_len = 0;
	first_len = 0;
	for(i = 0; i<12; i++) {
		if(ip->i_block[i] == 0) { //cannot find .
			return -1;
		}
		ext2->read_block(ip->i_block[i], sbuf);
		cp = sbuf;
		while(cp < sbuf + SDC_BLOCK_SIZE) {
			dp = (DIR *)cp;
			if(found == 0 && dp->inode!=0 && strcmp(dp->name, name)==0){
				//(2).1. if (first and only entry in a data block){
				if(dp->rec_len == SDC_BLOCK_SIZE){
					dp->inode = 0;
					ext2->write_block(ip->i_block[i], sbuf);
					return 0;
				}
				//(2).2. else if LAST entry in block
				else if(precp != NULL && dp->rec_len + (cp - sbuf) == SDC_BLOCK_SIZE){
					dp = (DIR *)precp;
					dp->rec_len = SDC_BLOCK_SIZE - (precp - sbuf);
					ext2->write_block(ip->i_block[i], sbuf);
					return 0;
				}
				//(2).3. else: entry is first but not the only entry or in the middle of a block:
				else{
					found = 1;
					rec_len = dp->rec_len;
					first_len = cp-sbuf;
				}
			}
			if(found == 0)
				precp = cp;
			cp += dp->rec_len;
		}
		if(found == 1) {
			dp->rec_len += rec_len;			
			memcpy(cpbuf, sbuf, first_len);
			memcpy(cpbuf + first_len, sbuf + first_len + dp->rec_len, SDC_BLOCK_SIZE-(first_len+rec_len));
			ext2->write_block(ip->i_block[i], cpbuf);
			return 0;
		}
	}
	return -1;
}

int32_t ext2_read(ext2_t* ext2, INODE* node, char *buf, int32_t nbytes, int32_t offset) {
	//(2) count = 0
	// avil = fileSize - OFT's offset // number of bytes still available in file.
	int32_t count_read = 0;
	char *cq = buf;
	int32_t avil = node->i_size - offset;
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	if(nbytes > SDC_BLOCK_SIZE)
		nbytes = SDC_BLOCK_SIZE;
	//(3)
	/*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
		lbk       = oftp->offset / SDC_BLOCK_SIZE;
		start_byte = oftp->offset % SDC_BLOCK_SIZE;*/	
	lbk = offset / SDC_BLOCK_SIZE;
	start_byte = offset % SDC_BLOCK_SIZE;
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

	char readbuf[SDC_BLOCK_SIZE];
	ext2->read_block(blk, readbuf);
	char *cp = readbuf + start_byte;
	remain = SDC_BLOCK_SIZE - start_byte;
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

INODE* get_node(ext2_t* ext2, int32_t ino, char* buf) {
	ext2->read_block(ext2->iblock + ((ino-1) / 8), buf);    // read block inode of me
	return (INODE *)buf + ((ino-1) % 8);
}

void put_node(ext2_t* ext2, int32_t ino, INODE *node) {
	int32_t blk = (ino-1)/8 + ext2->iblock;
	int32_t offset = (ino-1)%8;
	char buf[SDC_BLOCK_SIZE];
	ext2->read_block(blk, buf);
	INODE *ip = (INODE *)buf + offset;
	*ip = *node;
	ext2->write_block(blk, buf);	
}

int32_t ext2_create_dir(ext2_t* ext2, INODE* father_inp, const char *base, int32_t owner) { //mode file or dir
	int32_t ino, i, blk;
	char buf[SDC_BLOCK_SIZE];

	ino = ext2_ialloc(ext2);
	blk = ext2_balloc(ext2);

	INODE* inp = get_node(ext2, ino, buf);
	inp->i_mode = 0;//TODO EXT2_DIR_MODE;
	inp->i_uid  = owner & 0xffff;
	inp->i_gid  = (owner >> 16) & 0xffff;
	inp->i_size = 0;	        // Size in bytes 
	inp->i_links_count = 0;	  //
	inp->i_atime = 0;         // TODO Set last access to current time
	inp->i_ctime = 0;         // TODO  Set creation to current time
	inp->i_mtime = 0;         // TODO Set last modified to current time
	inp->i_blocks = SDC_BLOCK_SIZE / 512; // # of 512-byte blocks reserved for this inode 
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
	char buf[SDC_BLOCK_SIZE];

	ino = ext2_ialloc(ext2);

	INODE* inp = get_node(ext2, ino, buf);
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

int32_t ext2_write(ext2_t* ext2, INODE* node, char *data, int32_t nbytes, int32_t offset) {
 	char buf [SDC_BLOCK_SIZE];
	char *cq = data;
	char *cp;
	//(2)
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	int32_t nbytes_copy = 0;
	//(3)
	/*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
		lbk       = oftp->offset / SDC_BLOCK_SIZE;
		start_byte = oftp->offset % SDC_BLOCK_SIZE;*/	
	lbk = offset / SDC_BLOCK_SIZE;
	start_byte = offset % SDC_BLOCK_SIZE;
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
			memset(indirect_buf, 0, SDC_BLOCK_SIZE);
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
	remain = SDC_BLOCK_SIZE - start_byte;
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
	char buf[SDC_BLOCK_SIZE];

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			ext2->read_block(ip->i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;
			while (cp < &buf[SDC_BLOCK_SIZE]){
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

#define MAX_DIR_DEPTH 4
int32_t ext2_getino(ext2_t* ext2, const char* filename) {
	char buf[SDC_BLOCK_SIZE];
	int32_t depth, i, ino, u;
	char name[MAX_DIR_DEPTH][64];
	INODE *ip;

	if(strcmp(filename, "/") == 0)
		return 2; //ino 2 for root;

	depth = 0;
	while(true) {
		char* hold = name[depth];
		u = 0;
		if(*filename == '/') filename++; //skip '/'

		while(*filename != '/') {
			hold[u] = *filename;
			u++;
			filename++;
			if(*filename == 0 || u >= 63)
				break;
		}
		hold[u] = 0;
		depth++;
		if(*filename != 0)
			filename++;
		else
			break;
	}

	if(ext2->read_block(ext2->iblock, buf) != 0) // read first inode block
		return -1;
	ip = (INODE *)buf + 1;   // ip->root inode #2
	/* serach for system name */
	for (i=0; i<depth; i++) {
		ino = search(ext2, ip, name[i]);
		if (ino < 0) {
			return -1;
		}
		if(ext2->read_block(ext2->iblock+(ino-1/8), buf) != 0) { // read block inode of me
			return -1;
		}
		ip = (INODE *)buf + ((ino-1) % 8);
	}
	return ino;
}

int32_t ext2_unlink(ext2_t* ext2, const char* fname) {
	char buf[SDC_BLOCK_SIZE];
	tstr_t* dir = tstr_new("", MFS);
	tstr_t* name = tstr_new("", MFS);
	fs_parse_name(fname, dir, name);

	int32_t fino = ext2_getino(ext2, CS(dir));
	if(fino < 0) {
		tstr_free(dir);
		tstr_free(name);
		return -1;
	}
	INODE* fnode = get_node(ext2, fino, buf);
	tstr_free(dir);
	if(fnode == NULL) {
		tstr_free(name);
		return -1;
	}
	int32_t ino = search(ext2, fnode, CS(name));
	ext2_rm_child(ext2, fnode, CS(name));
	tstr_free(name);
	put_node(ext2, fino, fnode);

	if(ino < 0)
		return -1;
	INODE* node = get_node(ext2, ino, buf);
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
		bdealloc(ext2, node->i_block[i]);
	}
	//}
	idealloc(ext2, ino);
	//node->dirty = 1;
	put_node(ext2, ino, node);	
	return 0;
}

int32_t ext2_init(ext2_t* ext2, read_block_func_t read_block, write_block_func_t write_block) {
	char buf[SDC_BLOCK_SIZE];
	ext2->read_block = read_block;
	ext2->write_block = write_block;

	ext2->read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	ext2->nblocks = sp->s_blocks_count;
	ext2->ninodes = sp->s_inodes_count;
	
	/* read blk#2 to get group descriptor 0 */
	ext2->read_block(2, buf);
	GD* gp = (GD *)buf;
	ext2->iblock = (uint16_t)gp->bg_inode_table;
	ext2->bmap = gp->bg_block_bitmap;
	ext2->imap = gp->bg_inode_bitmap;
	return 0;
}
