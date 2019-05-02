#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <dev/devserv.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <device.h>

typedef struct {
	void* data;
} node_data_t;

static int32_t sramdisk_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
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
		info.size = new_size;
	}	
	char* p  = (char*)data->data;
	if(p == NULL)
		return -1;
	memcpy(p+seek, buf, size);
	fs_ninfo_update(node, &info);
	return size;
}

static int32_t sramdisk_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0 || info.data == NULL)
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

static int32_t sramdisk_open(uint32_t node, int32_t flags) {
	(void)flags;
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0)
		return -1;
	if(info.type == FS_TYPE_DIR)
		return 0;
	if(info.data == NULL) {
		info.data = malloc(sizeof(node_data_t));
		memset(info.data, 0, sizeof(node_data_t));
		fs_ninfo_update(node, &info);
	}
	return 0;
}

static int32_t sramdisk_close(uint32_t node) {
	fs_info_t info;
	if(fs_ninfo(node, &info) != 0)
		return -1;
	if(info.type == FS_TYPE_DIR)
		return 0;
	if(info.data == NULL) {
		info.data = malloc(sizeof(node_data_t));
		memset(info.data, 0, sizeof(node_data_t));
		fs_ninfo_update(node, &info);
	}
	return 0;
}

int main() {
	device_t dev = {0};
	dev.open = sramdisk_open;
	dev.close = sramdisk_close;
	dev.read = sramdisk_read;
	dev.write = sramdisk_write;

	//dev_run(&dev, "dev.sramdisk", 0, "/sramdisk", false);
	dev_run(&dev, "dev.sramdisk", 0, "/tmp", false);
	return 0;
}
