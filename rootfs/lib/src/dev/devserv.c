#include <dev/devserv.h>
#include <kserv.h>
#include <unistd.h>
#include <ipc.h>
#include <stdlib.h>
#include <kstring.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <proto.h>
#include <stdio.h>

static void do_open(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	int32_t fd = proto_read_int(proto);
	int32_t flags = proto_read_int(proto);
	proto_free(proto);

	if(fd < 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->open != NULL)
		ret = dev->open(pkg->pid, fd, flags);	
	ipc_send(pkg->id, pkg->type, &ret, 4);
}

static void do_close(device_t* dev, package_t* pkg) { 
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid != pkg->pid) { //only accept from vfsd
		return;
	}

	fs_info_t* info = (fs_info_t*)get_pkg_data(pkg);
	if(info == NULL || info->node == 0) {
		return;
	}
	if(dev->close != NULL)
		dev->close(info);
}

static void do_remove(device_t* dev, package_t* pkg) { 
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid != pkg->pid) //only accept from vfsd
		return;

	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* fname = proto_read_str(proto);
	fs_info_t* info = proto_read(proto, NULL);
	proto_free(proto);
	if(info == NULL || fname[0] == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t res = -1;
	if(dev->remove != NULL)
		res = dev->remove(info, fname);
	if(res != 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void do_dma(device_t* dev, package_t* pkg) { 
	int32_t fd = *(int32_t*)get_pkg_data(pkg);
	if(fd < 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t dma = 0;
	uint32_t size = 0;
	if(dev->dma != NULL) {
		dma = dev->dma(pkg->pid, fd, &size);
	}
		
	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, dma);
	proto_add_int(proto, (int32_t)size);
	ipc_send(pkg->id, pkg->type, proto->data, proto->size);
	proto_free(proto);
}

static void do_flush(device_t* dev, package_t* pkg) { 
	int32_t fd = *(int32_t*)get_pkg_data(pkg);
	if(fd < 0) {
		return;
	}
	if(dev->flush != NULL)
		dev->flush(pkg->pid, fd);
}

static void do_add(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	const char* fname = proto_read_str(proto);
	proto_free(proto);

	if(fname[0] == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = -1;
	if(dev->add != NULL)
		ret = dev->add(pkg->pid, fname);
	if(ret < 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else 
		ipc_send(pkg->id, pkg->type, NULL, 0);
}

static void do_write(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	int32_t fd = proto_read_int(proto);
	uint32_t size;
	void* p = proto_read(proto, &size);
	int32_t seek = proto_read_int(proto);
	proto_free(proto);

	if(fd < 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->write != NULL)
		ret = dev->write(pkg->pid, fd, p, size, seek);	
	if(ret < 0 && errno == EAGAIN)
		ipc_send(pkg->id, PKG_TYPE_AGAIN, NULL, 0);
	else 
		ipc_send(pkg->id, pkg->type, &ret, 4);
}

static void do_read(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	int32_t fd = proto_read_int(proto);
	uint32_t size = (uint32_t)proto_read_int(proto);
	uint32_t seek = (uint32_t)proto_read_int(proto);
	proto_free(proto);
	
	if(fd < 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(size == 0) {
		ipc_send(pkg->id, pkg->type, NULL, 0);
		return;
	}

	char* buf = (char*)malloc(size);
	int32_t ret = 0;
	if(dev->read != NULL)
		ret = dev->read(pkg->pid, fd, buf, size, seek);	

	if(ret < 0) {
		if(errno == EAGAIN)
			ipc_send(pkg->id, PKG_TYPE_AGAIN, NULL, 0);
		else
			ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	}
	else {
		ipc_send(pkg->id, pkg->type, buf, ret);
	}
	free(buf);
}

static void do_ctrl(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	int32_t fd = (int32_t)proto_read_int(proto);
	uint32_t cmd = (uint32_t)proto_read_int(proto);
	uint32_t size;
	void* p = proto_read(proto, &size);
	proto_free(proto);

	if(fd < 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	char* ret_data = NULL;
	int32_t ret = -1;
	if(dev->ctrl != NULL)
		ret_data = dev->ctrl(pkg->pid, fd, cmd, p, size, &ret);	

	if(ret < 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, ret_data, ret);
	if(ret_data != NULL)
		free(ret_data);
}

static void handle(package_t* pkg, void* p) {
	device_t* dev = (device_t*)p;
	switch(pkg->type) {
		case FS_OPEN:
			do_open(dev, pkg);
			break;
		case FS_CLOSE:
			do_close(dev, pkg);
			break;
		case FS_REMOVE:
			do_remove(dev, pkg);
			break;
		case FS_WRITE:
			do_write(dev, pkg);
			break;
		case FS_READ:
			do_read(dev, pkg);
			break;
		case FS_CTRL:
			do_ctrl(dev, pkg);
			break;
		case FS_DMA:
			do_dma(dev, pkg);
			break;
		case FS_FLUSH:
			do_flush(dev, pkg);
			break;
		case FS_ADD:
			do_add(dev, pkg);
			break;
	}
}

static void unmount(device_t* dev, const char* fname) {
	if(vfs_unmount(fname) != 0)
		return;
	if(dev->unmount != NULL)
		dev->unmount(getpid(), fname);
}

void dev_run(device_t* dev, int32_t argc, char** argv) {
	if(argc < 5) {
		printf("driver:%s arguments missed!\n", argv[0]);
		return;
	}
	const char* dev_name = argv[1];
	uint32_t index = atoi(argv[2]);
	const char* node_name = argv[3];
	bool file = true;

	if(argv[4][0] == 'D' || argv[4][0] == 'd')
		file = false;

	if(vfs_mount(node_name, dev_name, index, file) != 0)
		return;
	if(dev->mount != NULL) {
		if(dev->mount(node_name, index) != 0)
			return;
	}
	printf("(%s mounted to vfs:%s)\n", dev_name, node_name);

	if(kserv_register(dev_name) != 0) {
    printf("Panic: '%s' service register failed!\n", dev_name);
		unmount(dev, node_name);
		return;
	}

	if(kserv_ready() != 0) {
    printf("Panic: '%s' service can not get ready!\n", dev_name);
		unmount(dev, node_name);
		return;
	}

	kserv_run(handle, dev);
	unmount(dev, node_name);
}
