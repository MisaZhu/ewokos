#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <dev/devserv.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <device.h>

typedef struct {
	uint32_t size;
	int32_t dirty;
	void* data;
} node_data_t;

static int32_t sramdisk_write(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0 ||
			info.data == NULL)
		return -1;
	node_data_t* data = (node_data_t*)info.data;

	uint32_t new_size = seek+size;
	if(new_size == 0)
		return 0;
	if(new_size > info.size) {
		void * tmp = malloc(new_size);
		if(data->data != NULL) {
			memcpy(tmp, data->data, info.size);
			free(data->data);
		}
		data->data = tmp;
		data->dirty = 1;
		info.size = new_size;
		data->size = new_size;
	}	
	char* p  = (char*)data->data;
	if(p == NULL)
		return -1;
	memcpy(p+seek, buf, size);
	fs_update(&info);
	return size;
}

static int32_t sramdisk_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0 ||
			info.data == NULL)
		return -1;
	node_data_t* data = (node_data_t*)info.data;

	int32_t fsize = 0;
	const char* p = NULL;
	if(data->data == NULL)
		return -1;

	fsize = info.size;
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

static int32_t sramdisk_add(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	return 0;
}

static int32_t sramdisk_open(int32_t pid, int32_t fd, int32_t flags) {
	(void)flags;
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0)
		return -1;
	if(info.type == FS_TYPE_DIR)
		return 0;
	if(info.data == NULL) {
		info.data = malloc(sizeof(node_data_t));
		memset(info.data, 0, sizeof(node_data_t));
		fs_update(&info);
	}
	return 0;
}

static int32_t sramdisk_remove(fs_info_t* info, const char* fname) {
	if(info == NULL || fname == NULL || fname[0] == 0)
		return -1;
	if(info->type == FS_TYPE_DIR)
		return -1;
	if(syscall2(SYSCALL_PFILE_GET_REF, info->node, 2) > 0) 
		return -1;

	if(info->data != NULL) {
		node_data_t* data = (node_data_t*)info->data;
		if(data->data != NULL)
			free(data->data);
		free(info->data);
	}
	return 0;
}

static int32_t sramdisk_close(int32_t pid, int32_t fd, fs_info_t* info) {
	(void)pid;
	(void)fd;

	if(info == NULL)
		return 0;
	if(info->type == FS_TYPE_DIR)
		return 0;
	if(info->data != NULL) {
		node_data_t* data = (node_data_t*)info->data;
		if(data->dirty == 1) {
			info->size = data->size;
			fs_update(info);
		}
		data->dirty = 0;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	device_t dev = {0};
	dev.add = sramdisk_add;
	dev.open = sramdisk_open;
	dev.close = sramdisk_close;
	dev.read = sramdisk_read;
	dev.write = sramdisk_write;
	dev.remove = sramdisk_remove;
	
	dev_run(&dev, argc, argv);
	return 0;
}
