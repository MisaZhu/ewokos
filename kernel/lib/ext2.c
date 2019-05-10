#include <ext2.h>
#include <kstring.h>

static int32_t search(INODE *ip, const char *name, read_block_func_t read_block, char* buf) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			read_block(ip->i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;
			while (cp < &buf[SDC_BLOCK_SIZE]){
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if (strcmp(dp->name, name) == 0 ){
					return(dp->inode);
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

char* ext2_load(const char* filename, int32_t *sz, malloc_func_t mlc, read_block_func_t read_block, char* buf1, char* buf2) {
	int32_t depth, i, me, iblk, count, u, blk12;
	char name[MAX_DIR_DEPTH][64];
	char *ret, *addr;
	uint32_t *up;
	GD *gp;
	INODE *ip;
	*sz = -1;

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

	/* read blk#2 to get group descriptor 0 */
	if(read_block(2, buf1) != 0)
		return NULL;
	gp = (GD *)buf1;
	iblk = (uint16_t)gp->bg_inode_table;

	if(read_block(iblk, buf1) != 0) // read first inode block
		return NULL;
	ip = (INODE *)buf1 + 1;   // ip->root inode #2

	/* serach for system name */
	for (i=0; i<depth; i++) {
		me = search(ip, name[i], read_block, buf2) - 1;
		if (me < 0) 
			return NULL;
		if(read_block(iblk+(me/8), buf1) != 0)  // read block inode of me
			return NULL;
		ip = (INODE *)buf1 + (me % 8);
	}

	*sz = ip->i_size;
	int32_t mlc_size = ALIGN_UP(*sz, SDC_BLOCK_SIZE);
	blk12 = ip->i_block[12];
	addr = (char *)mlc(mlc_size);
	ret = addr;
	/* read indirect block into b2 */

	count = 0;
	for (i=0; i<12 && count<(*sz); i++){
		if (ip->i_block[i] == 0)
			break;
		read_block(ip->i_block[i], addr);
		addr += SDC_BLOCK_SIZE;
		count += SDC_BLOCK_SIZE;
	}

	if (blk12) { // only if file has indirect blocks
		read_block(blk12, buf1);
		up = (uint32_t *)buf1;      
		while(*up && count < (*sz)){
			read_block(*up, addr);
			addr += SDC_BLOCK_SIZE;
			up++; 
			count += SDC_BLOCK_SIZE;
		}
	}
	return ret;
}

int32_t ext2_write(INODE* node, int32_t offset, char *buf, int32_t nbytes, read_block_func_t read_block, write_block_func_t write_block, char* wbuf) {
	char *cq = buf;
	char *cp;
	//(2)
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	int32_t nbytes_copy = 0;
	//(3)
	while(nbytes > 0){
		/*(4) Compute LOGICAL BLOCK number lbk and start_byte in that block from offset;
			lbk       = oftp->offset / SDC_BLOCK_SIZE;
			start_byte = oftp->offset % SDC_BLOCK_SIZE;*/	
		lbk = offset / SDC_BLOCK_SIZE;
		start_byte = offset % SDC_BLOCK_SIZE;
		//5.1 direct
		if(lbk < 12){
			//if its the first one of the new block, allocatea new one
			if(node->i_block[lbk] == 0){
				//node->i_block[lbk] = balloc("dev_name")//TODO MUST ALLOCATE a block
			}
			blk = node->i_block[lbk];
		}
		//5.2 indirect 
		else if(lbk >=12 && lbk < 256+12){
			int32_t* indirect_buf = (int32_t*)wbuf;
			//if its the first one of the new block, allocatea new one
			if(node->i_block[12] == 0){
				//node->i_block[12] = balloc("dev_name"); //TODO
				memset(indirect_buf, 0, SDC_BLOCK_SIZE);
				write_block(node->i_block[12], (char*)indirect_buf);
			}
			read_block(node->i_block[12], (char*)indirect_buf);
			if(indirect_buf[lbk-12] == 0){
				//indirect_buf[lbk-12] = balloc("dev_name"); //TODO
				write_block(node->i_block[12], (char*)indirect_buf);
			}
			blk = indirect_buf[lbk-12]; 
		}
		else{
			break;
		}
		
		read_block(blk, wbuf);
		cp = wbuf + start_byte;
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
			if(offset > node->i_size){
				node->i_size += min;
			}			
			if(nbytes<=0){
				break;
			}
		}
		write_block(blk, wbuf);
		// loop back to while to write more .... until nbytes are written
	}
	return nbytes_copy;
}
