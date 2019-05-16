#include <kstring.h>
#include <dev/basic_dev.h>
#include <mm/kmalloc.h>
#include <sdread.h>

typedef struct ext2_group_desc {
	uint32_t	bg_block_bitmap;	/* Blocks bitmap block */
	uint32_t	bg_inode_bitmap;	/* Inodes bitmap block */
	uint32_t	bg_inode_table;		/* Inodes table block */
	uint16_t	bg_free_blocks_count;	/* Free blocks count */
	uint16_t	bg_free_inodes_count;	/* Free inodes count */
	uint16_t	bg_used_dirs_count;	/* Directories count */
	uint16_t	bg_pad;
	uint32_t	bg_reserved[3];
} GD;

typedef struct ext2_inode {
	uint16_t	i_mode;		/* File mode */
	uint16_t	i_uid;		/* Owner Uid */
	uint32_t	i_size;		/* Size in bytes */
	uint32_t	i_atime;	/* Access time */
	uint32_t	i_ctime;	/* Creation time */
	uint32_t	i_mtime;	/* Modification time */
	uint32_t	i_dtime;	/* Deletion Time */
	uint16_t	i_gid;		/* Group Id */
	uint16_t	i_links_count;	/* Links count */
	uint32_t	i_blocks;	/* Blocks count */
	uint32_t	i_flags;	/* File flags */
	uint32_t  dummy;
	uint32_t	i_block[15];    /* Pointers to blocks */
	uint32_t  pad[5];         /* inode size MUST be 128 bytes */
	uint32_t	i_date;         /* MTX date */
	uint32_t	i_time;         /* MTX time */
} INODE;

typedef struct ext2_dir_entry_2 {
	uint32_t	inode;			/* Inode number */
	uint16_t	rec_len;		/* Directory entry length */
	uint8_t	name_len;		/* Name length */
	uint8_t	file_type;
	char	name[255];      	/* File name */
} DIR;

#define BLOCK_SIZE 1024

static int32_t read_block(int32_t block, char* buf) {
	uint32_t typeid = dev_typeid(DEV_SDC, 0);
	if(dev_block_read(typeid, block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = dev_block_read_done(typeid, buf);
		if(res == 0)
			break;
	}
	return 0;
}

static int32_t search(INODE *ip, const char *name, char* buf) {
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
					return(dp->inode - 1);
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

char* ext2_load(const char* filename, int32_t *sz) {
	char* buf1 = (char*)km_alloc(BLOCK_SIZE);
	int32_t depth, i, ino, iblk, count, u, blk12;
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
	if(read_block(2, buf1) != 0) {
		km_free(buf1);
		return NULL;
	}
	gp = (GD *)buf1;
	iblk = (uint16_t)gp->bg_inode_table;

	if(read_block(iblk, buf1) != 0) // read first inode block
		return NULL;
	ip = (INODE *)buf1 + 1;   // ip->root inode #2

	char* buf2 = (char*)km_alloc(BLOCK_SIZE);
	/* serach for system name */
	for (i=0; i<depth; i++) {
		ino = search(ip, name[i], buf2);
		if (ino < 0) {
			km_free(buf1);
			km_free(buf2);
			return NULL;
		}
		if(read_block(iblk+(ino/8), buf1) != 0) { // read block inode of me
			km_free(buf1);
			km_free(buf2);
			return NULL;
		}
		ip = (INODE *)buf1 + (ino % 8);
	}

	*sz = ip->i_size;
	int32_t mlc_size = ALIGN_UP(*sz, BLOCK_SIZE);
	blk12 = ip->i_block[12];
	addr = (char *)km_alloc(mlc_size);
	ret = addr;
	/* read indirect block into b2 */

	count = 0;
	for (i=0; i<12 && count<(*sz); i++){
		if (ip->i_block[i] == 0)
			break;
		read_block(ip->i_block[i], addr);
		addr += BLOCK_SIZE;
		count += BLOCK_SIZE;
	}

	if (blk12) { // only if file has indirect blocks
		read_block(blk12, buf1);
		up = (uint32_t *)buf1;      
		while(*up && count < (*sz)){
			read_block(*up, addr);
			addr += BLOCK_SIZE;
			up++; 
			count += BLOCK_SIZE;
		}
	}

	km_free(buf1);
	km_free(buf2);
	return ret;
}

char* from_sd(const char *filename, int32_t* sz) { 
	return ext2_load(filename, sz);
}
