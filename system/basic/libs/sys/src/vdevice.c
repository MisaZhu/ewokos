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
#include <sys/lockc.h>

#ifdef __cplusplus
extern "C" {
#endif


static void do_open(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	fsinfo_t info;
	int oflag;
	int fd = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	oflag = proto_read_int(in);
	
	int res = 0;
	if(fd >= 0 && dev != NULL && dev->open != NULL) {
		if(dev->open(fd, from_pid, &info, oflag, p) != 0) {
			res = -1;
		}
	}
	PF->addi(out, res);
}

static void do_close(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)out;
	fsinfo_t info;
	int fd = proto_read_int(in);
	int pid = proto_read_int(in);
	if(pid < 0)
		pid = from_pid;
	proto_read_to(in, &info, sizeof(fsinfo_t));

	if(dev != NULL && dev->close != NULL) {
		dev->close(fd, pid, &info, p);
	}
}

static void do_read(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, offset, shm_id;
	fsinfo_t info;
	int fd = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	size = proto_read_int(in);
	offset = proto_read_int(in);
	shm_id = proto_read_int(in);

	if(dev != NULL && dev->read != NULL) {
		void* buf;
		if(shm_id < 0)
			buf = malloc(size);
		else
			buf = shm_map(shm_id);

		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->read(fd, from_pid, &info, buf, size, offset, p);
			PF->addi(out, size);
			if(size > 0) {
				if(shm_id < 0) {
					PF->add(out, buf, size);
				}
			}

			if(shm_id >= 0)
				shm_unmap(shm_id);
			else
				free(buf);
		}
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_write(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int32_t size, offset, shm_id;
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	offset = proto_read_int(in);
	shm_id = proto_read_int(in);
	
	if(dev != NULL && dev->write != NULL) {
		void* data;
		if(shm_id < 0)
			data = proto_read(in, &size);
		else {
			size = proto_read_int(in);
			data = shm_map(shm_id);
		}

		if(data == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->write(fd, from_pid, &info, data, size, offset, p);
			PF->addi(out, size);
		}
		if(shm_id >= 0)
			shm_unmap(shm_id);
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_read_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, index, shm_id;
	size = proto_read_int(in);
	index = proto_read_int(in);
	shm_id = proto_read_int(in);

	if(dev != NULL && dev->read_block != NULL) {
		void* buf;
		if(shm_id < 0)
			buf = malloc(size);
		else
			buf = shm_map(shm_id);
		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->read_block(from_pid, buf, size, index, p);
			PF->addi(out, size);
			if(size > 0) {
				if(shm_id < 0) {
					PF->add(out, buf, size);
				}
			}

			if(shm_id >= 0)
				shm_unmap(shm_id);
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
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	int id = -1;	
	int size = 0;
	if(dev != NULL && dev->dma != NULL) {
		id = dev->dma(fd, from_pid, &info, &size, p);
	}
	PF->addi(out, id)->addi(out, size);
}

static void do_fcntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	fsinfo_t info;
	int fd = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	int32_t cmd = proto_read_int(in);

	proto_t arg_in, arg_out;
	PF->init(&arg_out, NULL, 0);

	int32_t arg_size;
	void* arg_data = proto_read(in, &arg_size);
	PF->init(&arg_in, arg_data, arg_size);

	int res = -1;
	if(dev != NULL && dev->fcntl != NULL) {
		res = dev->fcntl(fd, from_pid, &info, cmd, &arg_in, &arg_out, p);
	}
	PF->clear(&arg_in);

	PF->addi(out, res)->add(out, arg_out.data, arg_out.size);
	PF->clear(&arg_out);
}

static void do_flush(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	if(dev != NULL && dev->flush != NULL) {
		dev->flush(fd, from_pid, &info, p);
	}
	PF->addi(out, 0);
}

static void do_create(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info_to, info;
	proto_read_to(in, &info_to, sizeof(fsinfo_t));
	proto_read_to(in, &info, sizeof(fsinfo_t));

	int res = 0;
	if(dev != NULL && dev->create != NULL) {
		res = dev->create(&info_to, &info, p);
	}

	PF->addi(out, res)->add(out, &info, sizeof(fsinfo_t));
}

static void do_unlink(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info_to, info;
	proto_read_to(in, &info_to, sizeof(fsinfo_t));
	const char* fname = proto_read_str(in);

	int res = 0;
	if(dev != NULL && dev->unlink != NULL) {
		res = dev->unlink(&info, fname, p);
	}
	PF->addi(out, res);
}

static void do_clear_buffer(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info;
	proto_read_to(in, &info, sizeof(fsinfo_t));

	int res = -1;
	if(dev != NULL && dev->clear_buffer != NULL) {
		res = dev->clear_buffer(&info, p);
	}

	PF->addi(out, res);
}

static void do_dev_cntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int cmd = proto_read_int(in);
	int32_t  sz;
	void* data = proto_read(in, &sz);
	
	proto_t in_arg, ret;
	PF->init(&in_arg, NULL, 0);
	PF->init(&ret, NULL, 0);
	PF->addi(out, -1);

	if(data != NULL) 
		PF->copy(&in_arg, data, sz);

	if(dev != NULL && dev->dev_cntl != NULL) {
		if(dev->dev_cntl(from_pid, cmd, &in_arg, &ret, p) == 0) {
			PF->clear(out)->addi(out, 0)->add(out, ret.data, ret.size);
		}
	}
	PF->clear(&in_arg);
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

	if(vfs_mount(mnt_point, &info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type) {
	if(dev == NULL)
		return -1;
	
	fsinfo_t mnt_point_info;
	if(mnt_point != NULL) {
		if(strcmp(mnt_point, "/") != 0)
			vfs_create(mnt_point, &mnt_point_info, mnt_type);
		else
			vfs_get(mnt_point, &mnt_point_info);

		if(do_mount(dev, &mnt_point_info, mnt_type) != 0)
			return -1;
	}

	int ipc_flags = 0;

	if(dev->loop_step != NULL) 
		ipc_flags |= IPC_NONBLOCK;
	ipc_serv_run(handle, dev, ipc_flags);

	while(1) {
		if(dev->loop_step != NULL) {
			ipc_lock();
			dev->loop_step(dev->extra_data);
			ipc_unlock();
			usleep(10000);
		}
		else {
			sleep(1);
		}
	}

	if(mnt_point != NULL && dev->umount != NULL) {
		return dev->umount(&mnt_point_info, dev->extra_data);
	}
	vfs_umount(&mnt_point_info);
	return 0;
}

int dev_cntl_by_pid(int pid, int cmd, proto_t* in, proto_t* out) {
	proto_t in_arg, ret;
	PF->init(&ret, NULL, 0);

	PF->init(&in_arg, NULL, 0)->
		addi(&in_arg, cmd);

	if(in != NULL)
		PF->add(&in_arg, in->data, in->size);

	int res = ipc_call(pid, FS_CMD_DEV_CNTL, &in_arg, &ret);
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
		if(out != NULL) {
			int32_t sz;
			void *data = proto_read(&ret, &sz);
			PF->copy(out, data, sz);
		}
	}
	PF->clear(&ret);
	return res;
}

int dev_get_pid(const char* fname) {
	fsinfo_t info;
	if(vfs_get(fname, &info) != 0)
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

