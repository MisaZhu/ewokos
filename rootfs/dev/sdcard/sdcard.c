#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <dev/devserv.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <ext2.h>
#include <device.h>

static int32_t _iblock = 12;
static int32_t _typeid =  dev_typeid(DEV_SDC, 0);

static int32_t read_block(int32_t block, char* buf) {
	if(syscall2(SYSCALL_DEV_BLOCK_READ, _typeid, block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = syscall2(SYSCALL_DEV_BLOCK_READ_DONE, _typeid, (int32_t)buf);
		if(res == 0)
			break;
		sleep(0);
	}
	return 0;
}

static int32_t write_block(int32_t block, char* buf) {
	if(syscall3(SYSCALL_DEV_BLOCK_WRITE, _typeid, block, (int32_t)buf) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = syscall1(SYSCALL_DEV_BLOCK_WRITE_DONE, _typeid);
		if(res == 0)
			break;
		sleep(0);
	}
	return 0;
}

static int32_t tst_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}
/*
static int32_t clr_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] &= ~(1 << j);
	return 0;
}
*/

static int32_t set_bit(char *buf, int32_t bit) {
	int32_t i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] |= (1 << j);
	return 0;
}

static void dec_free_blocks() {
	char* buf = (char*)malloc(SDC_BLOCK_SIZE);
	// inc free block count in SUPER and GD
	read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	write_block(1, buf);

	read_block(2, buf);
	GD* gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	write_block(2, buf);

	free(buf);
}

/*
static void dec_free_inodes() {
	char* buf = (char*)malloc(SDC_BLOCK_SIZE);
	read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	write_block(1, buf);

	read_block(2, buf);
	GD* gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	write_block(2, buf);  
	free(buf);
}

static uint32_t ext2_ialloc() {
	char* buf = (char*)malloc(SDC_BLOCK_SIZE);
	read_block(1, buf);
	SUPER* sp = (SUPER *)buf;
	int32_t ninodes = sp->s_inodes_count;

	read_block(2, buf);
	GD* gp = (GD *)buf;
	int32_t imap = gp->bg_inode_bitmap;
	// get inode Bitmap into buf
	read_block(imap, buf);

	int32_t i;
	for (i=0; i < ninodes; i++){
		if (tst_bit(buf, i)==0){
			set_bit(buf, i);
			write_block(imap, buf);
			// update free inode count in SUPER and GD
			dec_free_inodes();
			free(buf);
			return (i+1);
		}
	}
	free(buf);
	return 0;
} 
*/
static int32_t ext2_balloc() {
 	char* buf1 = (char*)malloc(SDC_BLOCK_SIZE);
	// inc free block count in SUPER and GD
	read_block(1, buf1);
	SUPER* sp = (SUPER *)buf1;
	int32_t nblocks = sp->s_blocks_count;
	
	/* read blk#2 to get group descriptor 0 */
	if(read_block(2, buf1) != 0) {
		free(buf1);
		return -1;
	}
	GD* gp = (GD *)buf1;
	int32_t bmap = gp->bg_block_bitmap;
	int32_t i;  
 	char* buf2 = (char*)malloc(SDC_BLOCK_SIZE);

	read_block(bmap, buf1);
	for (i = 0; i < nblocks; i++) {
		if (tst_bit(buf1, i) == 0) {
			set_bit(buf1, i);
			dec_free_blocks();
			write_block(bmap, buf1);
			return i+1;
		}
	}
	free(buf1);
	free(buf2);
	return 0;
}

static int32_t ext2_read(INODE* node, char *buf, int32_t nbytes, int32_t offset) {
	//(2) count = 0
	// avil = fileSize - OFT's offset // number of bytes still available in file.
	int32_t count_read = 0;
	char *cq = buf;
	int32_t avil = node->i_size - offset;
	int32_t blk =0, lbk = 0, start_byte = 0, remain = 0;
	if(nbytes > SDC_BLOCK_SIZE)
		nbytes = SDC_BLOCK_SIZE;
	//(3)
	while(nbytes && avil) {
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
			read_block(node->i_block[12], (char*)indirect_buf);
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
			read_block(node->i_block[13], (char*)double_buf1);
			int32_t double_buf2[256];
			read_block(double_buf1[num], (char*)double_buf2);
			blk = double_buf2[pos_offset];
		}

		char readbuf[SDC_BLOCK_SIZE];
		read_block(blk, readbuf);
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
	}
	return count_read;
}	


int32_t ext2_write(INODE* node, char *data, int32_t nbytes, int32_t offset) {
 	char* buf1 = (char*)malloc(SDC_BLOCK_SIZE);
	char *cq = data;
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
				node->i_block[lbk] = ext2_balloc();
			}
			blk = node->i_block[lbk];
		}
		//5.2 indirect 
		else if(lbk >=12 && lbk < 256+12){
			int32_t* indirect_buf = (int32_t*)buf1;
			//if its the first one of the new block, allocatea new one
			if(node->i_block[12] == 0){
				node->i_block[12] = ext2_balloc();
				memset(indirect_buf, 0, SDC_BLOCK_SIZE);
				write_block(node->i_block[12], (char*)indirect_buf);
			}
			read_block(node->i_block[12], (char*)indirect_buf);
			if(indirect_buf[lbk-12] == 0){
				indirect_buf[lbk-12] = ext2_balloc();
				write_block(node->i_block[12], (char*)indirect_buf);
			}
			blk = indirect_buf[lbk-12]; 
		}
		else{
			break;
		}
		
		read_block(blk, buf1);
		cp = buf1 + start_byte;
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
		write_block(blk, buf1);
		// loop back to while to write more .... until nbytes are written
	}
	free(buf1);
	return nbytes_copy;
}

typedef struct {
	INODE node;
	int32_t ino;
	void* data;
} ext2_node_data_t;

static inline INODE* read_node(int32_t ino, char* buf) {
	read_block(_iblock+(ino/8), buf);    // read block inode of me
	return (INODE *)buf + (ino % 8);
}

static void put_node(int32_t ino, INODE *node, char* buf){
	int blk = (ino-1)/8 + _iblock;
	int offset = (ino-1)%8;

	read_block(blk, buf);
	INODE *ip = (INODE *)buf + offset;
	*ip = *node;
	write_block(blk, buf);	
}

static int32_t add_nodes(INODE *ip, uint32_t node) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;
	char* buf1 = (char*)malloc(SDC_BLOCK_SIZE);
	char* buf2 = (char*)malloc(SDC_BLOCK_SIZE);

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			read_block(ip->i_block[i], buf1);
			dp = (DIR *)buf1;
			cp = buf1;

			if(dp->inode == 0)
				continue;

			while (cp < &buf1[SDC_BLOCK_SIZE]){
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if(strcmp(dp->name, ".") != 0 && strcmp(dp->name, "..") != 0) {
					int32_t ino = dp->inode - 1;
					INODE* ip_node = read_node(ino, buf2);
					ext2_node_data_t* data = (ext2_node_data_t*)malloc(sizeof(ext2_node_data_t));
					data->data = NULL;
					data->ino = ino;
					memcpy(&data->node, ip_node, sizeof(INODE));

					if(dp->file_type == 2) {//director
						uint32_t node_add = vfs_add(node, dp->name, VFS_DIR_SIZE, data);
						add_nodes(&data->node, node_add);
					}
					else if(dp->file_type == 1) { //file
						vfs_add(node, dp->name, data->node.i_size, data);
					}
				}
				//add node
				dp->name[dp->name_len] = c; // restore that last byte
				cp += dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}

	free(buf1);
	free(buf2);
	return 0;
}

static int32_t sdcard_mount(uint32_t node, int32_t index) {
	(void)index;
	GD *gp;
	INODE *ip;
	char* buf = (char*)malloc(SDC_BLOCK_SIZE);

	/* read blk#2 to get group descriptor 0 */
	read_block(2, buf);
	gp = (GD *)buf;
	_iblock = (uint16_t)gp->bg_inode_table;
	read_block(_iblock, buf);       // read first inode block
	ip = (INODE *)buf + 1;   // ip->root inode #2

	add_nodes(ip, node);
	free(buf);
	return 0;
}

int32_t sdcard_open(uint32_t node, int32_t flags) {
	(void)flags;

	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	return 0;
}

int32_t sdcard_close(fs_info_t* info) {
	if(info == NULL || info->data == NULL || info->node == 0)
	 	return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info->data;
	char* buf = (char*)malloc(SDC_BLOCK_SIZE);
	put_node(data->ino, &data->node, buf);
	free(buf);
	return 0;
}

int32_t sdcard_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info.data;
	return ext2_read(&data->node, buf, size, seek);
}

int32_t sdcard_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info.data;
	return ext2_write(&data->node, buf, size, seek);
}

int32_t main(int32_t argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = sdcard_mount;
	dev.open = sdcard_open;
	dev.read = sdcard_read;
	dev.write = sdcard_write;
	dev.close = sdcard_close;

	dev_run(&dev, argc, argv);
	return 0;
}
