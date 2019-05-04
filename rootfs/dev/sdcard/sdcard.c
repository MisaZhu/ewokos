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

typedef struct {
	INODE node;
	void* data;
} ext2_node_data_t;

static inline INODE* get_node(int32_t me, uint16_t iblk, char* buf) {
	read_block(iblk+(me/8), buf);    // read block inode of me
	return (INODE *)buf + (me % 8);
}

static int32_t add_nodes(INODE *ip, uint16_t iblk, uint32_t node) {
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
					int32_t me = dp->inode - 1;
					INODE* ip_node = get_node(me, iblk, buf2);
					ext2_node_data_t* data = (ext2_node_data_t*)malloc(sizeof(ext2_node_data_t));
					data->data = NULL;
					memcpy(&data->node, ip_node, sizeof(INODE));

					if(dp->file_type == 2) {//director
						uint32_t node_add = vfs_add(node, dp->name, VFS_DIR_SIZE, data);
						add_nodes(&data->node, iblk, node_add);
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

	add_nodes(ip, _iblock, node);

	free(buf);
	return 0;
}

static char* ext2_read(INODE* ip, int32_t* sz) { 
	*sz = ip->i_size;
	int32_t blk12 = ip->i_block[12];
	int32_t mlc_size = ALIGN_UP(*sz, SDC_BLOCK_SIZE);
	char* addr = (char *)malloc(mlc_size);
	char* ret = addr;
	uint32_t *up;
	/* read indirect block into b2 */

	int32_t i, count = 0;
	for (i=0; i<12 && count<(*sz); i++){
		if (ip->i_block[i] == 0)
			break;
		if(read_block(ip->i_block[i], addr) != 0) {
			free(addr);
			return NULL;
		}
		addr += SDC_BLOCK_SIZE;
		count += SDC_BLOCK_SIZE;
	}

	if (blk12) { // only if file has indirect blocks
		char* buf = (char*)malloc(SDC_BLOCK_SIZE);
		read_block(blk12, buf);
		up = (uint32_t *)buf;      
		while(*up && count<(*sz)){
			if(read_block(*up, addr) != 0) {
				free(addr);
				free(buf);
				return NULL;
			}
			addr += SDC_BLOCK_SIZE;
			up++;
			count += SDC_BLOCK_SIZE;
		}
		free(buf);
	}
	return ret;
}

int32_t sdcard_open(uint32_t node, int32_t flags) {
	(void)flags;

	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	if(info.type == FS_TYPE_DIR)
		return 0;

	ext2_node_data_t* data = (ext2_node_data_t*)info.data;

	int32_t fsize = 0;
	const char* p = NULL;
	if(data->data != NULL) {
		fsize = data->node.i_size;
		p = (const char*)data->data;
	}
	else {
		p = ext2_read(&data->node, &fsize);
		if(p == NULL || fsize == 0)
			return -1;
		data->data = (void*)p;
	}
	return 0;
}

int32_t sdcard_close(fs_info_t* info) {
	if(info == NULL || info->data == NULL || info->node == 0)
		return -1;
	if(info->type == FS_TYPE_DIR)
		return 0;
	if(syscall2(SYSCALL_PFILE_GET_REF, info->node, 2) > 0) 
		return 0;

	ext2_node_data_t* data = (ext2_node_data_t*)info->data;
	if(data->data != NULL) {
		free(data->data);
		data->data = NULL;
	}
	return 0;
}

int32_t sdcard_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info.data;

	int32_t fsize = 0;
	const char* p = NULL;
	if(data->data == NULL) {
		return -1;
	}

	fsize = data->node.i_size;
	p = (const char*)data->data;
	int32_t rest_size = fsize - seek; /*seek*/
	if(rest_size <= 0) {
		return 0;
	}

	if(size > (uint32_t)rest_size)
		size = rest_size;
	memcpy(buf, p+seek, size);
	return size;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = sdcard_mount;
	dev.open = sdcard_open;
	dev.read = sdcard_read;
	dev.close = sdcard_close;

	dev_run(&dev, argc, argv);
	return 0;
}
