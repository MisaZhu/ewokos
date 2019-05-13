#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <dev/devserv.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <ext2fs.h>
#include <device.h>

static ext2_t _ext2;
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

typedef struct {
	INODE node;
	int32_t ino;
	int32_t dirty;
	void* data;
} ext2_node_data_t;

static int32_t add_nodes(INODE *ip, uint32_t node) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;
	char* buf1 = (char*)malloc(SDC_BLOCK_SIZE);
	char* buf2 = (char*)malloc(SDC_BLOCK_SIZE);

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			_ext2.read_block(ip->i_block[i], buf1);
			dp = (DIR *)buf1;
			cp = buf1;

			if(dp->inode == 0)
				continue;

			while (cp < &buf1[SDC_BLOCK_SIZE]){
				c = dp->name[dp->name_len];  // save last byte
				dp->name[dp->name_len] = 0;   
				if(strcmp(dp->name, ".") != 0 && strcmp(dp->name, "..") != 0) {
					int32_t ino = dp->inode;
					INODE* ip_node = get_node(&_ext2, ino, buf2);
					ext2_node_data_t* data = (ext2_node_data_t*)malloc(sizeof(ext2_node_data_t));
					data->data = NULL;
					data->dirty = 0;
					data->ino = ino;
					memcpy(&data->node, ip_node, sizeof(INODE));

					if(dp->file_type == 2) {//director
						uint32_t node_add = vfs_add(node, dp->name, VFS_DIR_SIZE, data);
						add_nodes(&data->node, node_add);
					}
					else if(dp->file_type == 1) {//file
						//printf("%s: %d\n",dp->name, data->node.i_size);
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
	ext2_init(&_ext2, read_block, write_block);

	char* buf = (char*)malloc(SDC_BLOCK_SIZE);
	_ext2.read_block(_ext2.iblock, buf);       // read first inode block
	INODE *ip = (INODE *)buf + 1;   // ip->root inode #2
	add_nodes(ip, node);
	free(buf);
	return 0;
}

static int32_t sdcard_open(uint32_t node, int32_t flags) {
	(void)flags;

	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	return 0;
}

static int32_t sdcard_close(fs_info_t* info) {
	if(info == NULL || info->data == NULL || info->node == 0)
	 	return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info->data;
	if(info->type == FS_TYPE_FILE && data->dirty != 0) {
		put_node(&_ext2, data->ino, &data->node);
		info->size = data->node.i_size;
		fs_ninfo_update(info);
	}
	return 0;
}

static int32_t sdcard_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info.data;
	return ext2_read(&_ext2, &data->node, buf, size, seek);
}

static int32_t sdcard_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info.data;
	data->dirty = 1;
	return ext2_write(&_ext2, &data->node, buf, size, seek);
}

static int32_t sdcard_add(uint32_t father_node, uint32_t node, const char* name, uint32_t type) {
	fs_info_t father_info, info;
	if(fs_ninfo(father_node, &father_info) != 0 ||
			vfs_node_by_addr(node, &info) != 0)
		return -1;

	INODE* inp = NULL;
	char *sbuf = (char*)malloc(SDC_BLOCK_SIZE);

	if(father_info.data != NULL) {
		ext2_node_data_t* data = (ext2_node_data_t*)father_info.data;
		inp = &data->node;
	}
	else {
		inp = get_node(&_ext2, 2, sbuf);
	}
	int32_t ino;
	if(type == FS_TYPE_DIR)
		ino= ext2_create_dir(&_ext2, inp, name);
	else
		ino= ext2_create_file(&_ext2, inp, name);

	inp = get_node(&_ext2, ino, sbuf);
	ext2_node_data_t* data = (ext2_node_data_t*)malloc(sizeof(ext2_node_data_t));
	data->data = NULL;
	data->dirty = 0;
	data->ino = ino;
	memcpy(&data->node, inp, sizeof(INODE));
	free(sbuf);
	info.data = data;
	fs_ninfo_update(&info);
	return 0;
}

static int32_t sdcard_remove(const char* fname) {
	if(fname == NULL || fname[0] == 0)
		return -1;
	fs_info_t info;
	if(vfs_node_by_name(fname, &info) != 0 || info.type == FS_TYPE_DIR)
		return -1;
	if(syscall2(SYSCALL_PFILE_GET_REF, info.node, 2) > 0) 
		return -1;

	tstr_t* dir = tstr_new("", MFS);
	tstr_t* name = tstr_new("", MFS);
	fs_parse_name(fname, dir, name);

	int32_t res = -1;
	fs_info_t finfo;
	if(vfs_node_by_name(CS(dir), &finfo) != 0 || vfs_node_by_name(CS(name), &info) != 0) {
		tstr_free(dir); 
		tstr_free(name); 
		return -1;
	}
	if(info.data != NULL) {
		char sbuf [SDC_BLOCK_SIZE];
		INODE *inp;
		int32_t fino;
		if(finfo.data == NULL) { //root
			inp = get_node(&_ext2, 2, sbuf);
			fino = 2;
		}
		else  {
			ext2_node_data_t* fdata = (ext2_node_data_t*)finfo.data;
			inp = &fdata->node;
			fino = fdata->ino;
		}

		ext2_node_data_t* data = (ext2_node_data_t*)info.data;
		res = ext2_unlink(&_ext2, inp, fino, &data->node, data->ino, CS(name));
	}
	tstr_free(dir); 
	tstr_free(name); 
	return res;
}

int32_t main(int32_t argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = sdcard_mount;
	dev.add = sdcard_add;
	dev.open = sdcard_open;
	dev.read = sdcard_read;
	dev.write = sdcard_write;
	dev.close = sdcard_close;
	dev.remove = sdcard_remove;

	dev_run(&dev, argc, argv);
	return 0;
}
