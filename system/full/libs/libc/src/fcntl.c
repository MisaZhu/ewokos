#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <stddef.h>
#include <string.h>

int open(const char* fname, int oflag) {
	int fd = -1;
	fsinfo_t info;

	if(vfs_get(fname, &info) != 0) {
		if((oflag & O_CREAT) != 0) {
			if(vfs_create(fname, &info, FS_TYPE_FILE) != 0)
				return -1;
		}
		else  {
			return -1;	
		}	
	}

	uint32_t ufid = 0;
	fd = vfs_open(&info, oflag, &ufid);
	if(fd < 0)
		return -1;
	
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, fd)->
		addi(&in, ufid)->
		add(&in, &info, sizeof(fsinfo_t))->
		addi(&in, oflag);

	if(ipc_call(info.mount_pid, FS_CMD_OPEN, &in, &out) != 0 ||
			proto_read_int(&out) != 0) {
		vfs_close(fd);
		fd = -1;
	}

	PF->clear(&in);
	PF->clear(&out);
	return fd;
}

int finfo(const char* fname, proto_t* out) {
	fsinfo_t info;

	if(vfs_get(fname, &info) != 0) {
		return -1;
	}

	proto_t in, ret;
	PF->init(out, NULL, 0);
	PF->init(&ret, NULL, 0);

	PF->init(&in, NULL, 0)->
		adds(&in, fname)->
		add(&in, &info, sizeof(fsinfo_t));

	int res = ipc_call(info.mount_pid, FS_CMD_INFO, &in, &ret);
	PF->clear(&in);
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
	return res;
}

void close(int fd) {
	fsinfo_t info;
	uint32_t ufid = 0;
	if(vfs_get_by_fd(fd, &ufid, &info) != 0)
		return;

	proto_t in;
	PF->init(&in, NULL, 0)->
		addi(&in, fd)->
		addi(&in, ufid)->
		add(&in, &info, sizeof(fsinfo_t));

	ipc_call(info.mount_pid, FS_CMD_CLOSE, &in, NULL);
	PF->clear(&in);
	vfs_close(fd);
}

int dma(int fd, int* size) {
	fsinfo_t info;
	uint32_t ufid = 0;
	if(vfs_get_by_fd(fd, &ufid, &info) != 0)
		return -1;
	
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, fd)->
		addi(&in, ufid)->
		add(&in, &info, sizeof(fsinfo_t));

	int shm_id = -1;
	if(ipc_call(info.mount_pid, FS_CMD_DMA, &in, &out) == 0) {
		shm_id = proto_read_int(&out);
		if(size != NULL)
			*size = proto_read_int(&out);
	}
	PF->clear(&in);
	PF->clear(&out);
	return shm_id;
}

void flush(int fd) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, NULL, &info) != 0)
		return;
	
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, fd)->
		add(&in, &info, sizeof(fsinfo_t));
	ipc_call(info.mount_pid, FS_CMD_FLUSH, &in, &out);
	PF->clear(&in);
	PF->clear(&out);
}

int fcntl_raw(int fd, int cmd, proto_t* arg_in, proto_t* arg_out) {
	fsinfo_t info;
	uint32_t ufid;
	if(vfs_get_by_fd(fd, &ufid, &info) != 0)
		return -1;
	
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, fd)->
		addi(&in, ufid)->
		add(&in, &info, sizeof(fsinfo_t))->
		addi(&in, cmd);
	if(arg_in == NULL)
		PF->add(&in, NULL, 0);
	else
		PF->add(&in, arg_in->data, arg_in->size);

	int res = -1;
	if(ipc_call(info.mount_pid, FS_CMD_CNTL, &in, &out) == 0) {
		res = proto_read_int(&out);
		if(arg_out != NULL) {
			int32_t sz;
			void *p = proto_read(&out, &sz);
			PF->copy(arg_out, p, sz);
		}
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}
