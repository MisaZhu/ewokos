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
	uint32_t node = (uint32_t)proto_read_int(proto);
	int32_t flags = proto_read_int(proto);
	proto_free(proto);

	if(node == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->open != NULL)
		ret = dev->open(node, flags);	
	ipc_send(pkg->id, pkg->type, &ret, 4);
}

static void do_close(device_t* dev, package_t* pkg) { 
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	if(node == 0) {
		return;
	}

	if(dev->close != NULL)
		dev->close(node);
}

static void do_dma(device_t* dev, package_t* pkg) { 
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);

	if(node == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t dma = 0;
	uint32_t size = 0;
	if(dev->dma != NULL) {
		dma = dev->dma(node, &size);
	}
		
	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, dma);
	proto_add_int(proto, (int32_t)size);

	ipc_send(pkg->id, pkg->type, proto->data, proto->size);
	proto_free(proto);
}

static void do_flush(device_t* dev, package_t* pkg) { 
	uint32_t node = *(uint32_t*)get_pkg_data(pkg);
	if(node == 0) {
		return;
	}

	if(dev->flush != NULL)
		dev->flush(node);
}

static void do_add(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node = (uint32_t)proto_read_int(proto);
	const char* name = proto_read_str(proto);
	proto_free(proto);

	if(node == 0 || name[0] == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->add != NULL)
		ret = dev->add(node, name);	
	ipc_send(pkg->id, pkg->type, &ret, 4);
}

static void doWrite(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node = (uint32_t)proto_read_int(proto);
	uint32_t size;
	void* p = proto_read(proto, &size);
	int32_t seek = proto_read_int(proto);
	proto_free(proto);

	if(node == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->write != NULL)
		ret = dev->write(node, p, size, seek);	

	ipc_send(pkg->id, pkg->type, &ret, 4);
}

static void do_read(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node = (uint32_t)proto_read_int(proto);
	uint32_t size = (uint32_t)proto_read_int(proto);
	uint32_t seek = (uint32_t)proto_read_int(proto);
	
	if(node == 0) {
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
		ret = dev->read(node, buf, size, seek);	

	if(ret < 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, buf, ret);
	free(buf);
}

static void do_ctrl(device_t* dev, package_t* pkg) { 
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node = (uint32_t)proto_read_int(proto);
	uint32_t cmd = (uint32_t)proto_read_int(proto);
	uint32_t size;
	void* p = proto_read(proto, &size);
	proto_free(proto);

	if(node == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	p = NULL;
	int32_t ret = -1;
	if(dev->ctrl != NULL)
		p = dev->ctrl(node, cmd, p, size, &ret);	

	if(ret < 0)
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipc_send(pkg->id, pkg->type, p, ret);
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
		case FS_WRITE:
			doWrite(dev, pkg);
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

void dev_run(device_t* dev, const char* dev_name, uint32_t index, const char* node_name, bool file) {
	if(kserv_get_pid(dev_name) >= 0) {
		printf("%s device service has been running!\n", dev_name);
		exit(0);
	}

	uint32_t node = vfs_mount(node_name, dev_name, index, file);
	if(node == 0)
		return;
	
	if(dev->mount != NULL) {
		if(dev->mount(node, index) != 0)
			return;
	}
	printf("(%s mounted to vfs:%s)\n", dev_name, node_name);
	kserv_run(dev_name, handle, dev);

	if(vfs_unmount(node) != 0)
		return;

	if(dev->unmount != NULL) {
		dev->unmount(node);
	}
}
