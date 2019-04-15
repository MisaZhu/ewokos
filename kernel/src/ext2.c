#include <ext2.h>
#include <kstring.h>
#include <dev/sdc.h>
#include <mm/kmalloc.h>

static char buf1[SDC_BLOCK_SIZE];
static char buf2[SDC_BLOCK_SIZE];

static int32_t read_block(int32_t block, char* buf) {
	if(sdc_read_block(block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = sdc_read_done(buf);
		if(res == 0)
			break;
	}
	return 0;
}

static int32_t search(INODE *ip, const char *name) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			read_block(ip->i_block[i], buf2);
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

#define MAX_DIR_DEPTH 4

char* ext2_load(const char *filename, int32_t* sz) { 
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
	read_block(2, buf1);
	gp = (GD *)buf1;
	iblk = (uint16_t)gp->bg_inode_table;


	read_block(iblk, buf1);       // read first inode block
	ip = (INODE *)buf1 + 1;   // ip->root inode #2

	/* serach for system name */
	for (i=0; i<depth; i++) {
		me = search(ip, name[i]) - 1;
		if (me < 0) {
			return NULL;
		}
		read_block(iblk+(me/8), buf1);    // read block inode of me
		ip = (INODE *)buf1 + (me % 8);
	}

	*sz = ip->i_size;
	blk12 = ip->i_block[12];
	addr = (char *)km_alloc(*sz);
	ret = addr;
	/* read indirect block into b2 */

	count = 0;
	for (i=0; i<12; i++){
		if (ip->i_block[i] == 0)
			break;
		read_block(ip->i_block[i], addr);
		addr += SDC_BLOCK_SIZE;
		count += SDC_BLOCK_SIZE;
	}

	if (blk12) { // only if file has indirect blocks
		read_block(blk12, buf1);
		up = (uint32_t *)buf1;      
		while(*up){
			read_block(*up, addr); 
			addr += SDC_BLOCK_SIZE;
			up++; count += SDC_BLOCK_SIZE;
			if(count > (*sz - SDC_BLOCK_SIZE))
				break;
		}
	}
	return ret;
}
