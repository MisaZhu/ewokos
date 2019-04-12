#include <ext2.h>
#include <kstring.h>

#define BLKSIZE 1024
static char _buf1[BLKSIZE], _buf2[BLKSIZE];

static int32_t search(read_block_func_t read_func, INODE *ip, const char *name) {
	int32_t i; 
	char c, *cp;
	DIR  *dp; 

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			read_func(ip->i_block[i], _buf2);
			dp = (DIR *)_buf2;
			cp = _buf2;
			while (cp < &_buf2[BLKSIZE]){
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

char* ext2_load(const char *filename, read_block_func_t read_func, malloc_func_t mlc, int32_t* sz) { 
	int32_t depth, i, me, iblk, count, u;
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
	read_func(2, _buf1);
	gp = (GD *)_buf1;
	iblk = (uint16_t)gp->bg_inode_table;


	read_func(iblk, _buf1);       // read first inode block block
	ip = (INODE *)_buf1 + 1;   // ip->root inode #2

	/* serach for system name */
	for (i=0; i<depth; i++) {
		me = search(read_func, ip, name[i]) - 1;
		if (me < 0) 
			return NULL;
		read_func(iblk+(me/8), _buf1);    // read block inode of me
		ip = (INODE *)_buf1 + (me % 8);
	}

	*sz = ip->i_size;
	addr = (char *)mlc(*sz);
	ret = addr;
	/* read indirect block into b2 */
	if (ip->i_block[12])         // only if has indirect blocks 
		read_func(ip->i_block[12], _buf2);

	count = 0;
	for (i=0; i<12; i++){
		if (ip->i_block[i] == 0)
			break;
		read_func(ip->i_block[i], addr);
		addr += BLKSIZE;
		count += BLKSIZE;
	}

	if (ip->i_block[12]) { // only if file has indirect blocks
		up = (uint32_t *)_buf2;      
		while(*up){
			read_func(*up, addr); 
			addr += BLKSIZE;
			up++; count += BLKSIZE;
		}
	}
	return ret;
}  
