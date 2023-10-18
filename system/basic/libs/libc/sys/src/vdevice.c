#include <sys/vdevice.h>
#include <sys/vfs.h>
#include <sys/ipc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/signal.h>

#ifdef __cplusplus
extern "C" {
#endif

static void do_open(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int oflag;
	int fd = proto_read_int(in);
	uint32_t node = proto_read_int(in);
	oflag = proto_read_int(in);
	
	int res = 0;
	if(fd >= 0 && dev != NULL && dev->open != NULL) {
		if(dev->open(fd, from_pid, node, oflag, p) != 0) {
			res = -1;
		}
	}
	PF->addi(out, res);
}

static void do_close(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)out;
	int fd = proto_read_int(in);
	int pid = proto_read_int(in);
	if(pid < 0)
		pid = from_pid;
	uint32_t node = (uint32_t)proto_read_int(in);

	if(dev != NULL && dev->close != NULL) {
		dev->close(fd, pid, node, p);
	}
}

#define READ_BUF_SIZE 32
static void do_read(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, offset;
	int fd = proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);
	size = proto_read_int(in);
	offset = proto_read_int(in);
	void* shm = (void*)proto_read_int(in);
	char buffer[READ_BUF_SIZE];

	if(dev != NULL && dev->read != NULL) {
		void* buf;
		if(shm == NULL) {
			if(size > READ_BUF_SIZE)
				buf = malloc(size);
			else
				buf = buffer;
		}
		else
			buf = shm_map(shm);

		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->read(fd, from_pid, node, buf, size, offset, p);
			PF->addi(out, size);
			if(size > 0) {
				if(shm == NULL) {
					PF->add(out, buf, size);
				}
			}

			if(shm != NULL)
				shm_unmap(shm);
			else if(buf != buffer)
				free(buf);
		}
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_write(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int32_t size, offset;
	int fd = proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);
	offset = proto_read_int(in);
	void *shm = (void*)proto_read_int(in);
	
	if(dev != NULL && dev->write != NULL) {
		void* data;
		if(shm == NULL)
			data = proto_read(in, &size);
		else {
			size = proto_read_int(in);
			data = shm_map(shm);
		}

		if(data == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->write(fd, from_pid, node, data, size, offset, p);
			PF->addi(out, size);
		}
		if(shm != NULL)
			shm_unmap(shm);
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_read_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, index;
	size = proto_read_int(in);
	index = proto_read_int(in);
	void* shm = (void*)proto_read_int(in);

	if(dev != NULL && dev->read_block != NULL) {
		void* buf;
		if(shm == NULL)
			buf = malloc(size);
		else
			buf = shm_map(shm);
		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->read_block(from_pid, buf, size, index, p);
			PF->addi(out, size);
			if(size > 0) {
				if(shm == NULL) {
					PF->add(out, buf, size);
				}
			}

			if(shm != NULL)
				shm_unmap(shm);
			else
				free(buf);
		}
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_write_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int32_t size, index;
	void* data = proto_read(in, &size);
	index = proto_read_int(in);

	if(dev != NULL && dev->write_block != NULL) {
		size = dev->write_block(from_pid, data, size, index, p);
		PF->addi(out, size);
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_dma(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int fd = proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);

	int id = -1;	
	int size = 0;
	if(dev != NULL && dev->dma != NULL) {
		id = dev->dma(fd, from_pid, node, &size, p);
	}
	PF->addi(out, id)->addi(out, size);
}

static void do_fcntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int fd = proto_read_int(in);
	uint32_t node = proto_read_int(in);
	int32_t cmd = proto_read_int(in);

	proto_t arg_in, arg_out;
	PF->init(&arg_out);

	int32_t arg_size;
	void* arg_data = proto_read(in, &arg_size);
	PF->init_data(&arg_in, arg_data, arg_size);

	int res = -1;
	if(dev != NULL && dev->fcntl != NULL) {
		res = dev->fcntl(fd, from_pid, node, cmd, &arg_in, &arg_out, p);
	}
	PF->clear(&arg_in);

	PF->addi(out, res)->add(out, arg_out.data, arg_out.size);
	PF->clear(&arg_out);
}

static void do_flush(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info;
	int fd = proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);

	int ret = 0;
	if(dev != NULL && dev->flush != NULL) {
		ret = dev->flush(fd, from_pid, node, p);
	}
	PF->addi(out, ret);
}

static void do_create(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	uint32_t node_to = (uint32_t)proto_read_int(in);
	uint32_t node = (uint32_t)proto_read_int(in);

	fsinfo_t info;
	int res = -1;
	if(dev != NULL && dev->create != NULL)
		res = dev->create(node_to, node, p, &info);

	if(res == 0) {
		PF->addi(out, res)->add(out, &info, sizeof(fsinfo_t));
		return;
	}		
	PF->addi(out, -1);
}

static void do_unlink(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	uint32_t node = (uint32_t)proto_read_int(in);
	const char* fname = proto_read_str(in);

	int res = 0;
	if(dev != NULL && dev->unlink != NULL) {
		res = dev->unlink(node, fname, p);
	}
	PF->addi(out, res);
}

static void do_clear_buffer(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	uint32_t node = proto_read_int(in);

	int res = -1;
	if(dev != NULL && dev->clear_buffer != NULL)
		res = dev->clear_buffer(node, p);
	PF->addi(out, res);
}

static void do_dev_cntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	PF->addi(out, -1);
	if(dev == NULL || dev->dev_cntl == NULL)
		return;

	int cmd = proto_read_int(in);
	int32_t  sz;
	void* data = proto_read(in, &sz);
	
	proto_t in_arg, ret;
	PF->init(&in_arg);
	PF->init(&ret);
	if(data != NULL && sz > 0) 
		PF->copy(&in_arg, data, sz);

	if(dev->dev_cntl(from_pid, cmd, &in_arg, &ret, p) == 0) {
		PF->clear(out)->addi(out, 0)->add(out, ret.data, ret.size);
	}
	PF->clear(&in_arg);
	PF->clear(&ret);
}

static void do_interrupt(vdevice_t* dev, proto_t *in, void* p) {
	if(dev != NULL && dev->interrupt != NULL) {
		dev->interrupt(in, p);
	}
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	vdevice_t* dev = (vdevice_t*)p;
	if(dev == NULL)
		return;
	PF->clear(out);

	p = dev->extra_data;
	switch(cmd) {
	case FS_CMD_OPEN:
		do_open(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CLOSE:
		do_close(dev, from_pid, in, out, p);
		break;
	case FS_CMD_READ:
		do_read(dev, from_pid, in, out, p);
		break;
	case FS_CMD_WRITE:
		do_write(dev, from_pid, in, out, p);
		break;
	case FS_CMD_READ_BLOCK:
		do_read_block(dev, from_pid, in, out, p);
		break;
	case FS_CMD_WRITE_BLOCK:
		do_write_block(dev, from_pid, in, out, p);
		break;
	case FS_CMD_DMA:
		do_dma(dev, from_pid, in, out, p);
		break;
	case FS_CMD_FLUSH:
		do_flush(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CNTL:
		do_fcntl(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CREATE:
		do_create(dev, from_pid, in, out, p);
		break;
	case FS_CMD_UNLINK:
		do_unlink(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CLEAR_BUFFER:
		do_clear_buffer(dev, from_pid, in, out, p);
		break;
	case FS_CMD_INTERRUPT:
		do_interrupt(dev, in, p);
		break;
	case FS_CMD_DEV_CNTL:
		do_dev_cntl(dev, from_pid, in, out, p);
		break;
	}
}

static int do_mount(vdevice_t* dev, fsinfo_t* mnt_point, int type) {
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = type;
	vfs_new_node(&info);

	if(dev->mount != NULL) {
		if(dev->mount(&info, dev->extra_data) != 0) {
			vfs_del(&info);
			return -1;
		}
	}

	if(vfs_mount(mnt_point->node, info.node) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

static void sig_stop(int sig_no, void* p) {
  (void)sig_no;
  vdevice_t* dev = (vdevice_t*)p;
  dev->terminated = true;
}

int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type) {
	if(dev == NULL)
		return -1;
	sys_signal(SYS_SIG_STOP, sig_stop, dev);
	
	fsinfo_t mnt_point_info;
	if(mnt_point != NULL) {
		if(vfs_get_by_name(mnt_point, &mnt_point_info) != 0) {
			if(vfs_create(mnt_point, &mnt_point_info, mnt_type, true, true) != 0)
				return -1;
		}

		if(do_mount(dev, &mnt_point_info, mnt_type) != 0)
			return -1;
	}

	int ipc_flags = 0;

	if(dev->loop_step != NULL) 
		ipc_flags |= IPC_NON_BLOCK;
	ipc_serv_run(handle, dev->handled, dev, ipc_flags);

	while(!dev->terminated) {
		if(dev->loop_step != NULL) {
			dev->loop_step(dev->extra_data);
		}
		else {
			proc_block(getpid(), (uint32_t)dev);
		}
	}

	if(mnt_point != NULL && dev->umount != NULL) {
		dev->umount(&mnt_point_info, dev->extra_data);
	}
	vfs_umount(mnt_point_info.node);
	return 0;
}

int dev_cntl_by_pid(int pid, int cmd, proto_t* in, proto_t* out) {
	proto_t in_arg;
	PF->init(&in_arg)->
		addi(&in_arg, cmd);

	if(in != NULL)
		PF->add(&in_arg, in->data, in->size);

	int res = -1;
	if(out != NULL) {
		proto_t ret;
		PF->init(&ret);
		res = ipc_call(pid, FS_CMD_DEV_CNTL, &in_arg, &ret);
		PF->clear(&in_arg);
		if(res != 0) {
			PF->clear(&ret);
			return -1;
		}

		res = proto_read_int(&ret);
		if(res != 0) {
			res = -1;
		}
		else {
			int32_t sz;
			void *data = proto_read(&ret, &sz);
			PF->copy(out, data, sz);
		}
		PF->clear(&ret);
	}
	else {
		res = ipc_call(pid, FS_CMD_DEV_CNTL, &in_arg, NULL);
		PF->clear(&in_arg);
	}
	return res;
}

int dev_get_pid(const char* fname) {
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0)
		return -1;
	return info.mount_pid;
}

int dev_cntl(const char* fname, int cmd, proto_t* in, proto_t* out) {
	int pid = dev_get_pid(fname);
	if(pid < 0)
		return -1;
	return dev_cntl_by_pid(pid, cmd, in, out);
}

#ifdef __cplusplus
}
#endif

