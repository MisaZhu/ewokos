#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <sys/sd.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <stdio.h>

static void add_file(fsinfo_t* node_to, const char* name, char* p, int32_t size) {
	fsinfo_t f;
	memset(&f, 0, sizeof(fsinfo_t));
	strcpy(f.name, name);
	f.type = FS_TYPE_FILE;
	f.size = size;
	f.data = (uint32_t)p;

	vfs_new_node(&f);
	vfs_add(node_to, &f);
}

static int add_dir(fsinfo_t* node_to, fsinfo_t* ret, const char* dn) {
	memset(ret, 0, sizeof(fsinfo_t));
	strcpy(ret->name, dn);
	ret->type = FS_TYPE_DIR;
	ret->size = 1024;
	vfs_new_node(ret);
	if(vfs_add(node_to, ret) != 0) {
		vfs_del(ret);
		return -1;
	}
	return 0;
}

extern const char* fs_data[];

static void b16_decode(const char *input, uint32_t input_len, char *output, uint32_t *output_len) {
	uint32_t i;
	for (i = 0; i < input_len / 2; i++) {
		uint8_t low = input[2 * i] - 'A';
		uint8_t high = input[2 * i + 1] - 'A';
		output[i] = low | (high << 4);
	}
	*output_len = input_len / 2;
}

static int32_t load(char* ret, int32_t i) {
	while(1) {
		const char* s = fs_data[i++];
		if(s == NULL)
			break;
		uint32_t sz = 0;
		b16_decode(s, strlen(s), ret , &sz);
		ret += sz;
	}
	return i;
}


static int32_t add_nodes(fsinfo_t* dinfo, int32_t i) {
	while(1) {
		//read name
		const char* s = fs_data[i++];
		if(s == NULL)
			return i;

		const char* sz = fs_data[i++];
		if(sz[0] == 'r') { //dir type
			fsinfo_t info;
			add_dir(dinfo, &info, s);
			i = add_nodes(&info, i);
		}
		else {
			int32_t size = atoi(sz);
			char* data = NULL;
			if(size > 0) {
				data = (char*)malloc(size);
			}
			i = load(data, i);
			add_file(dinfo, s, data, size);
		}
	}
	return i;
}

static int memfs_mount(fsinfo_t* info, void* p) {
	(void)p;
	add_nodes(info, 0);
	return 0;
}

static int memfs_read(int fd, int ufid, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)ufid;
	(void)from_pid;
	(void)p;

	char* data = (char*)info->data;
	int rsize = info->size - offset;
	if(rsize < size)
		size = rsize;
	if(size < 0)
		size = -1;

	data += offset;
	if(size > 0) 
		memcpy(buf, data, size);
	return size;	
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "rootfs(mem)");
	dev.mount = memfs_mount;
	dev.read = memfs_read;

	device_run(&dev, "/", FS_TYPE_DIR);
	return 0;
}
