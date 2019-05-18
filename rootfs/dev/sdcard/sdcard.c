#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <dev/devserv.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <vfs/ext2fs.h>
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

static int32_t add_nodes(INODE *ip, const char* dname) {
	int32_t i; 
	char c, *cp;
	DIR  *dp;
	char* buf1 = (char*)malloc(BLOCK_SIZE);
	char* buf2 = (char*)malloc(BLOCK_SIZE);

	for (i=0; i<12; i++){
		if ( ip->i_block[i] ){
			_ext2.read_block(ip->i_block[i], buf1);
			dp = (DIR *)buf1;
			cp = buf1;

			if(dp->inode == 0)
				continue;

			while (cp < &buf1[BLOCK_SIZE]){
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
						vfs_add(dname, dp->name, VFS_DIR_SIZE, data);
						tstr_t* dir_name = tstr_new(dname, MFS);
						if(strcmp(dname, "/") != 0)
							tstr_addc(dir_name, '/');
						tstr_add(dir_name, dp->name);
						add_nodes(&data->node, CS(dir_name));
						tstr_free(dir_name);
					}
					else if(dp->file_type == 1) {//file
						//printf("%s: %d\n",dp->name, data->node.i_size);
						vfs_add(dname, dp->name, data->node.i_size, data);
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

static int32_t sdcard_mount(const char* fname, int32_t index) {
	(void)index;
	ext2_init(&_ext2, read_block, write_block);

	char* buf = (char*)malloc(BLOCK_SIZE);
	_ext2.read_block(_ext2.iblock, buf);       // read first inode block
	INODE *ip = (INODE *)buf + 1;   // ip->root inode #2
	add_nodes(ip, fname);
	free(buf);
	return 0;
}

static int32_t sdcard_open(int32_t pid, int32_t fd, int32_t flags) {
	(void)pid;
	(void)fd;
	(void)flags;
	return 0;
}

static int32_t sdcard_close(int32_t pid, int32_t fd, fs_info_t* info) {
	(void)pid;
	(void)fd;

	if(info == NULL || info->data == NULL || info->node == 0)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info->data;
	if(info->type == FS_TYPE_FILE && data->dirty != 0) {
		put_node(&_ext2, data->ino, &data->node);
	}
	return 0;
}

static int32_t sdcard_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0 ||
			info.data == NULL)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info.data;
	return ext2_read(&_ext2, &data->node, buf, size, seek);
}

static int32_t sdcard_write(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0 ||
			info.data == NULL)
		return -1;
	ext2_node_data_t* data = (ext2_node_data_t*)info.data;
	int res =  ext2_write(&_ext2, &data->node, buf, size, seek);
	if(res > 0) {
		data->dirty = 1;
		info.size = data->node.i_size;
		fs_update(&info);
	}
	return res;
}

static int32_t sdcard_add(int32_t pid, const char* fname) {
	(void)pid;
	fs_info_t father_info, info;
	if(fs_finfo(fname, &info) != 0)
		return -1;
	tstr_t* dir = tstr_new("", MFS);
	tstr_t* name = tstr_new("", MFS);
	fs_parse_name(fname, dir, name);

	if(fs_finfo(CS(dir), &father_info) != 0) {
		tstr_free(dir);
		tstr_free(name);
		return -1;
	}
	tstr_free(dir);

	INODE* inp = NULL;
	char *sbuf = (char*)malloc(BLOCK_SIZE);
	if(father_info.data != NULL) {
		ext2_node_data_t* data = (ext2_node_data_t*)father_info.data;
		inp = &data->node;
	}
	else {
		inp = get_node(&_ext2, 2, sbuf);
	}
	
	int32_t ino;
	if(info.type == FS_TYPE_DIR)
		ino= ext2_create_dir(&_ext2, inp, CS(name), info.owner);
	else
		ino= ext2_create_file(&_ext2, inp, CS(name), info.owner);
	tstr_free(name);

	inp = get_node(&_ext2, ino, sbuf);
	ext2_node_data_t* data = (ext2_node_data_t*)malloc(sizeof(ext2_node_data_t));
	data->data = NULL;
	data->dirty = 0;
	data->ino = ino;
	memcpy(&data->node, inp, sizeof(INODE));
	free(sbuf);
	info.data = data;
	vfs_node_update(&info);
	return 0;
}

static int32_t sdcard_remove(fs_info_t* info, const char* fname) {
	if(info == NULL || fname == NULL || fname[0] == 0)
		return -1;
	if(syscall2(SYSCALL_PFILE_GET_REF, info->node, 2) > 0) 
		return -1;
	if(info->type == FS_TYPE_DIR) {
		if(info->size > 0) 
			return -1;
		return ext2_rmdir(&_ext2, fname);
	}
	return ext2_unlink(&_ext2, fname);
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
