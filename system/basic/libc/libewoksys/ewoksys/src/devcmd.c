#include <ewoksys/devcmd.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int dev_set(int dev_pid, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->add(&in, info, sizeof(fsinfo_t));

	int res = ipc_call(dev_pid, FS_CMD_SET, &in, &out);
	PF->clear(&in);
	if(res != 0) {
		PF->clear(&out);
		return res;
	}

	res = proto_read_int(&out);
	if(res != 0)
		errno = proto_read_int(&out);
	PF->clear(&out);
	return res;
}

int dev_stat(int dev_pid,  fsinfo_t* info, node_stat_t* stat) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&out);
	PF->init(&in)->add(&in, info, sizeof(fsinfo_t));
	int res = ipc_call(dev_pid, FS_CMD_STAT, &in, &out);
	PF->clear(&in);
	if(res != 0) {
		PF->clear(&out);
		return res;
	}
	res = proto_read_int(&out);
	if(res == 0)
		proto_read_to(&out, stat, sizeof(node_stat_t));

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int dev_unlink(int dev_pid, uint32_t node, const char* fname) {
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,s", node, fname);

	int res = ipc_call(dev_pid, FS_CMD_UNLINK, &in, &out);
	PF->clear(&in);
	if(res != 0) {
		PF->clear(&out);
		return res;
	}

	res = proto_read_int(&out);
	if(res != 0)
		errno = proto_read_int(&out);
	PF->clear(&out);
	return res;
}

int dev_open(int dev_pid, int fd, fsinfo_t* info, int oflag) {
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i,i", fd, info->node, oflag);

	int res = ipc_call(dev_pid, FS_CMD_OPEN, &in, &out);
	PF->clear(&in);
	if(res != 0) {
		PF->clear(&out);
		return res;
	}
	res =	proto_read_int(&out);
	if(res != 0)
		errno = proto_read_int(&out);
	else
		proto_read_to(&out, info, sizeof(fsinfo_t));

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

#define SHM_ON  256
#define SHM_MAX (1024*64)
int dev_read(int dev_pid, int fd, fsinfo_t* info, int32_t offset, void* buf, uint32_t size) {
	int32_t shm_id = -1;
	void* shm = NULL;
	if(size > SHM_ON) {
		if(size > SHM_MAX)
			size = SHM_MAX;
		key_t key = (info->node << 16) | getpid(); 
		shm_id = shmget(key, size, 0666|IPC_CREAT|IPC_EXCL);
		if(shm_id != -1)  {
			shm = shmat(shm_id, 0, 0);
			if(shm == NULL)
				return -1;
		}
	}

	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i,i,i,i", fd, info->node, size, offset, shm_id);

	int res = -1;
	if(ipc_call(dev_pid, FS_CMD_READ, &in, &out) == 0) {
		int rd = proto_read_int(&out);
		res = rd;
		if(rd > 0) {
			if(shm_id != -1 && shm != NULL)
				memcpy(buf, shm, rd);
			else
				proto_read_to(&out, buf, rd);
		}
	}
	PF->clear(&in);
	PF->clear(&out);
	if(shm != NULL)
		shmdt(shm);
	return res;
}

int dev_write(int dev_pid, int fd, fsinfo_t* info, int32_t offset, const void* buf, uint32_t size) {
	int32_t shm_id = -1;
	void* shm = NULL;
	if(size >= SHM_ON) {
		if(size > SHM_MAX)
			size = SHM_MAX;
		key_t key = (info->node << 16) | getpid(); 
		shm_id = shmget(key, size, 0666|IPC_CREAT|IPC_EXCL);
		if(shm_id != -1)  {
			shm = shmat(shm_id, 0, 0);
			if(shm == NULL)
				return -1;
			memcpy(shm, buf, size);
		}
	}

	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i,i,i", fd, info->node, offset, shm_id);
	if(shm_id == -1)
		PF->add(&in, buf, size);
	else
		PF->addi(&in, size);

	int res = -1;
	if(ipc_call(dev_pid, FS_CMD_WRITE, &in, &out) == 0) {
		int r = proto_read_int(&out);
		proto_read_to(&out, info, sizeof(fsinfo_t));
		res = r;
	}
	PF->clear(&in);
	PF->clear(&out);
	if(shm != NULL)
		shmdt(shm);
	return res;
}

int dev_create(int dev_pid, fsinfo_t* info_to, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i", info_to->node, info->node);

	int res = -1;
	if(ipc_call(dev_pid, FS_CMD_CREATE, &in, &out) == 0) {
		res = proto_read_int(&out);
		if(res == 0)
			proto_read_to(&out, info, sizeof(fsinfo_t));
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int dev_fcntl(int dev_pid, int fd, fsinfo_t* info, int cmd, proto_t* arg_in, proto_t* arg_out) {
	proto_t in;
	PF->format(&in, "i,i,i", fd, info->node, cmd);
	if(arg_in == NULL)
		PF->add(&in, NULL, 0);
	else
		PF->add(&in, arg_in->data, arg_in->size);

	int res = -1;
	proto_t out;
	PF->init(&out);
	if(arg_out == NULL)
		res = ipc_call(dev_pid, FS_CMD_CNTL, &in, NULL);
	else
		res = ipc_call(dev_pid, FS_CMD_CNTL, &in, &out);

	if(res == 0) {
		res = proto_read_int(&out);
		if(arg_out != NULL) {
			proto_read_to(&out, info, sizeof(fsinfo_t));
			int32_t sz;
			void *p = proto_read(&out, &sz);
			PF->copy(arg_out, p, sz);
		}
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int dev_flush(int dev_pid, int fd, uint32_t node, int8_t wait) {
	proto_t in;
	PF->format(&in, "i,i", fd, node);

	int res = -1;
	if(wait)
		res = ipc_call_wait(dev_pid, FS_CMD_FLUSH, &in);
	else
		res = ipc_call(dev_pid, FS_CMD_FLUSH, &in, NULL);
	PF->clear(&in);
	return res;
}

int dev_dma(int dev_pid, int fd, uint32_t node, int* size) {
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i", fd, node);

	int32_t shm_id = -1;
	if(ipc_call(dev_pid, FS_CMD_DMA, &in, &out) == 0) {
		shm_id = proto_read_int(&out);
		if(size != NULL)
			*size = proto_read_int(&out);
	}
	PF->clear(&in);
	PF->clear(&out);
	return shm_id;
}

int dev_write_block(int pid, const void* buf, uint32_t size, int32_t index) {
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "m,i", buf, size, index);

	int res = -1;
	if(ipc_call(pid, FS_CMD_WRITE_BLOCK, &in, &out) == 0) {
		int r = proto_read_int(&out);
		res = r;
		if(res == -2) {
			errno = EAGAIN;
			res = -1;
		}
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int dev_read_block(int pid, void* buf, uint32_t size, int32_t index) {
	key_t key = (((uint32_t)buf) << 16) | pid; 
	int32_t shm_id = shmget(key, size, 0666|IPC_CREAT);
	if(shm_id == -1) 
		return -1;
	void* shm = shmat(shm_id, 0, 0);
	if(shm == NULL)
		return 0;

	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i,i", size, index, shm_id);

	int res = -1;
	if(ipc_call(pid, FS_CMD_READ_BLOCK, &in, &out) == 0) {
		int rd = proto_read_int(&out);
		res = rd;
		if(rd > 0) {
			memcpy(buf, shm, rd);
		}
		if(res == VFS_ERR_RETRY) {
			errno = EAGAIN;
			res = -1;
		}
	}
	PF->clear(&in);
	PF->clear(&out);
	shmdt(shm);
	return res;
}

#ifdef __cplusplus
}
#endif

