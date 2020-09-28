#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/kprintf.h>
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

static const char* short_name(const char* name) {
	if(name[0] == 0)
		return name;

	int32_t len = strlen(name);
	const char* p = name+(len-1);
	while(true) {
		if(*p == '/') {
			p++;
			break;
		}
		if(p == name) {
			break;
		}
		p--;
	}
	return p;
}

static int32_t add_nodes(fsinfo_t* dinfo, int32_t i) {
	while(1) {
		char name[FS_FULL_NAME_MAX+1];
		int32_t sz = syscall3(SYS_KFS_GET, i, (int32_t)name, (int32_t)NULL);	
		if(sz < 0)
			return -1;
		const char* sname = short_name(name);
		if(sz == 0 && sname[0] == 0)  //end of dir
			return i+1;

		if(sz == 0 && sname[0] != 0) { //dir type
			fsinfo_t info;
			add_dir(dinfo, &info, sname);
			i = add_nodes(&info, i+1);
		}
		else {
			char* data = NULL;
			if(sz > 0) {
				data = (char*)malloc(sz);
				sz = syscall3(SYS_KFS_GET, i, 0, (int32_t)data);	
			}
			add_file(dinfo, sname, data, sz);
			i++;
		}
	}
	return i;
}

static int memfs_mount(fsinfo_t* info, void* p) {
	(void)p;
	add_nodes(info, 0);
	return 0;
}

static int memfs_read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
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

	if(getuid() >= 0) {
		kprintf(false, "this process can only loaded by kernel!\n");
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "rootfs(mem)");
	dev.mount = memfs_mount;
	dev.read = memfs_read;

	device_run(&dev, "/", FS_TYPE_DIR);
	return 0;
}
