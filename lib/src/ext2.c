#include <ext2.h>
#include <kstring.h>

static int32_t search(read_block_func_t read_func, INODE *ip, const char *name, char* buf2) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			read_func(ip->i_block[i], buf2);
			dp = (DIR *)buf2;
			cp = buf2;
			while (cp < &buf2[SDC_BLOCK_SIZE]){
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

#define MAX_DIR_DEPTH 32

char* ext2_load(const char *filename, read_block_func_t read_func, malloc_func_t mlc, free_func_t fr, int32_t* sz) { 
	char* buf1 = (char*)mlc(SDC_BLOCK_SIZE);
	char* buf2 = (char*)mlc(SDC_BLOCK_SIZE);
	int32_t depth, i, me, iblk, count, u, blk12;
	char name[64][MAX_DIR_DEPTH];
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
	read_func(2, buf1);
	gp = (GD *)buf1;
	iblk = (uint16_t)gp->bg_inode_table;


	read_func(iblk, buf1);       // read first inode block
	ip = (INODE *)buf1 + 1;   // ip->root inode #2

	/* serach for system name */
	for (i=0; i<depth; i++) {
		me = search(read_func,  ip, name[i], buf2) - 1;
		if (me < 0) {
			fr(buf1);
			fr(buf2);
			return NULL;
		}
		read_func(iblk+(me/8), buf1);    // read block inode of me
		ip = (INODE *)buf1 + (me % 8);
	}
	fr(buf2);

	*sz = ip->i_size;
	blk12 = ip->i_block[12];
	addr = (char *)mlc(*sz);
	ret = addr;
	/* read indirect block into b2 */

	count = 0;
	for (i=0; i<12; i++){
		if (ip->i_block[i] == 0)
			break;
		read_func(ip->i_block[i], addr);
		addr += SDC_BLOCK_SIZE;
		count += SDC_BLOCK_SIZE;
	}

	if (blk12) { // only if file has indirect blocks
		read_func(blk12, buf1);
		up = (uint32_t *)buf1;      
		while(*up){
			read_func(*up, addr); 
			addr += SDC_BLOCK_SIZE;
			up++; count += SDC_BLOCK_SIZE;
		}
	}
	fr(buf1);
	return ret;
}
