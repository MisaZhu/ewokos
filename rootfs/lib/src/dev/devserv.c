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

static int32_t do_open(int32_t pid, proto_t* in, device_t* dev) {
	int32_t fd = proto_read_int(in);
	int32_t flags = proto_read_int(in);

	if(fd < 0) {
		return -1;
	}

	int32_t ret = 0;
	if(dev->open != NULL)
		ret = dev->open(pid, fd, flags);	
	return ret;
}

static int32_t do_close(int32_t pid, proto_t* in, device_t* dev) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid != pid) { //only accept from vfsd
		return -1;
	}

	int32_t from_pid = proto_read_int(in);
	int32_t fd = proto_read_int(in);
	fs_info_t* info = (fs_info_t*)proto_read(in, NULL);
	if(info == NULL || info->node == 0) {
		return -1;
	}
	if(dev->close != NULL)
		dev->close(from_pid, fd, info);
	return 0;
}

static int32_t do_remove(int32_t pid, proto_t* in, device_t* dev) {
	int32_t serv_pid = kserv_get_by_name(VFS_NAME);	
	if(serv_pid != pid) { //only accept from vfsd
		return -1;
	}

	const char* fname = proto_read_str(in);
	fs_info_t* info = proto_read(in, NULL);

	errno = ENONE;
	if(info == NULL || fname[0] == 0) {
		return -1;
	}

	int32_t res = -1;
	if(dev->remove != NULL)
		res = dev->remove(info, fname);
	return res;
}

static int32_t do_dma(int32_t pid, proto_t* in, proto_t* out, device_t* dev) {
	int32_t fd = proto_read_int(in);
	if(fd < 0) {
		return -1;
	}

	int32_t dma = 0;
	uint32_t size = 0;
	if(dev->dma != NULL) {
		dma = dev->dma(pid, fd, &size);
	}
		
	proto_add_int(out, dma);
	proto_add_int(out, (int32_t)size);
	return 0;
}

static int32_t do_flush(int32_t pid, proto_t* in, device_t* dev) {
	int32_t fd = proto_read_int(in);
	if(fd < 0) {
		return -1;
	}
	if(dev->flush != NULL)
		dev->flush(pid, fd);
	return 0;
}

static int32_t do_add(int32_t pid, proto_t* in, device_t* dev) {
	const char* fname = proto_read_str(in);

	if(fname[0] == 0) {
		return -1;
	}

	int32_t ret = -1;
	if(dev->add != NULL)
		ret = dev->add(pid, fname);
	return ret;
}

static int32_t do_write(int32_t pid, proto_t* in, proto_t* out, device_t* dev) {
	int32_t fd = proto_read_int(in);
	uint32_t size;
	void* p = proto_read(in, &size);
	int32_t seek = proto_read_int(in);

	if(fd < 0) {
		return -1;
	}

	int32_t ret = 0;
	if(dev->write != NULL)
		ret = dev->write(pid, fd, p, size, seek);	
	if(ret <= 0 && errno == EAGAIN) {
		return -1;
	}
	proto_add_int(out, ret);
	return 0;
}

static int32_t do_read(int32_t pid, proto_t* in, proto_t* out, device_t* dev) {
	int32_t fd = proto_read_int(in);
	uint32_t size = (uint32_t)proto_read_int(in);
	uint32_t seek = (uint32_t)proto_read_int(in);
	
	if(fd < 0) {
		return -1;
	}

	if(size == 0) {
		proto_add(out, NULL, 0);
		return 0;
	}

	char* buf = (char*)malloc(size);
	int32_t ret = 0;
	if(dev->read != NULL)
		ret = dev->read(pid, fd, buf, size, seek);	

	if(ret <= 0 && errno == EAGAIN) {
		free(buf);
		return -1;
	}
	
	if(ret < 0)
		return -1;

	proto_add(out, buf, ret);
	free(buf);
	return 0;
}

static int32_t do_fctrl(int32_t pid, proto_t* in, proto_t* out, device_t* dev) {
	const char* fname = proto_read_str(in);
	uint32_t cmd = (uint32_t)proto_read_int(in);
	uint32_t size;
	void* p = proto_read(in, &size);

	proto_t* proto = proto_new(p, size);
	int32_t res = -1;
	if(dev->fctrl != NULL)
		res = dev->fctrl(pid, fname, cmd, proto, out);	
	proto_free(proto);
	return res;
}

static int32_t do_ctrl(int32_t pid, proto_t* in, proto_t* out, device_t* dev) {
	int32_t fd = (int32_t)proto_read_int(in);
	uint32_t cmd = (uint32_t)proto_read_int(in);
	uint32_t size;
	void* p = proto_read(in, &size);

	if(fd < 0) {
		return -1;
	}

	proto_t* proto = proto_new(p, size);
	int32_t res = -1;
	if(dev->ctrl != NULL)
		res = dev->ctrl(pid, fd, cmd, proto, out);	
	proto_free(proto);
	return res;
}

static int32_t ipccall(int32_t pid, int32_t call_id, proto_t* in, proto_t* out, void* p) {
	device_t* dev = (device_t*)p;
	switch(call_id) {
		case FS_OPEN:
			return do_open(pid, in, dev);
		case FS_CLOSE:
			return do_close(pid, in, dev);
		case FS_REMOVE:
			return do_remove(pid, in, dev);
		case FS_WRITE:
			return do_write(pid, in, out, dev);
		case FS_READ:
			return do_read(pid, in, out, dev);
		case FS_CTRL:
			return do_ctrl(pid, in, out, dev);
		case FS_FCTRL:
			return do_fctrl(pid, in, out, dev);
		case FS_DMA:
			return do_dma(pid, in, out, dev);
		case FS_FLUSH:
			return do_flush(pid, in, dev);
		case FS_ADD:
			return do_add(pid, in, dev);
	}
	return 0;
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

	kserv_run(ipccall, dev, dev->step, dev);
	unmount(dev, node_name);
}
