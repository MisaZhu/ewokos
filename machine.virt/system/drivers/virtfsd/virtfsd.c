#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <arch/virt/virtfs.h>

static void set_fsinfo_stat(node_stat_t* stat) {
	stat->atime = 0;
	stat->ctime = 0;
	stat->mtime = 0;
	stat->gid = 0;
	stat->uid = 0;
	stat->links_count = 1;
	stat->mode = 0;
	stat->size = 1024;
}

static void add_file(fsinfo_t* node_to, const char* name) {
	fsinfo_t f;
	memset(&f, 0, sizeof(fsinfo_t));
	strcpy(f.name, name);
	f.type = FS_TYPE_FILE;
	f.data = 0;
	set_fsinfo_stat(&f.stat);
	vfs_new_node(&f, node_to->node, false);
}

static int add_dir(fsinfo_t* info_to, fsinfo_t* ret, const char* dn) {
	memset(ret, 0, sizeof(fsinfo_t));
	strcpy(ret->name, dn);
	ret->type = FS_TYPE_DIR;
	ret->data = 0;
	set_fsinfo_stat(&ret->stat);
	if(vfs_new_node(ret, info_to->node, false) != 0)
		return -1;
	return 0;
}

static int _mount(fsinfo_t* info, void* p) {
	//klog("virtfsd mount %s\n", info->name);
	fsinfo_t ret;
	add_dir(info, &ret, "dir");
	add_file(info, "file");
	return 0;
}

static int _create(int pid, fsinfo_t* info_to, fsinfo_t* info, void* p) {
	//klog("virtfsd create %s\n", info->name);
	return 0;
}

static int _open(int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
	//klog("virtfsd open %s\n", info->name);
	return 0;	
}

static int _set(int from_pid, fsinfo_t* info, void* p) {
	//klog("virtfsd set %s\n", info->name);
	return 0;
}

static int _read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	//klog("virtfsd read %s\n", info->name);
	return size;	
}

static int _write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	//klog("virtfsd write %s\n", info->name);
	return size;	
}

static int _unlink(fsinfo_t* info, const char* fname, void* p) {
	//klog("virtfsd unlink %s\n", fname);
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	_mmio_base = mmio_map();
	virtfs_t fs = virtfs_init();
	if(fs == NULL) {
		klog("virtfs_init failed!\n");
		return -1;
	}
	virtfs_set_version(fs, "9P2000.L");
	virtfs_attach(fs, 2, 1, 0xffffffff, "root", "/");

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "sharedfs(virtfs)");

	dev.mount = _mount;
	dev.read = _read;
	dev.write = _write;
	dev.create = _create;
	dev.open = _open;
	dev.set = _set;
	dev.unlink = _unlink;
	
	dev.extra_data = &dev;
	device_run(&dev, "/mnt/share", FS_TYPE_DIR, 0771);
	return 0;
}