#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <stddef.h>
#include <string.h>

int dev_ping(int pid) {
	int res = -1;
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_PING);
	if(ipc_call(pid, &in, &out) == 0) {
		res = proto_read_int(&out);
	}
	proto_clear(&in);
	proto_clear(&out);
	return res;
}

int open(const char* fname, int oflag) {
	int res = -1;
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
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_OPEN);
	proto_add(&in, &info, sizeof(fsinfo_t));
	proto_add_int(&in, oflag);

	if(ipc_call(mount.pid, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	proto_clear(&in);
	proto_clear(&out);
	return res;
}

void close(int fd) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) == 0) {
		proto_t in;
		proto_init(&in, NULL, 0);

		proto_add_int(&in, FS_CMD_CLOSE);
		proto_add_int(&in, fd);
		proto_add(&in, &info, sizeof(fsinfo_t));

		ipc_call(mount.pid, &in, NULL);
		proto_clear(&in);
	}

	syscall1(SYS_VFS_PROC_CLOSE, fd);
}

int dma(int fd, int* size) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_DMA);
	proto_add_int(&in, fd);
	proto_add(&in, &info, sizeof(fsinfo_t));

	int shm_id = -1;
	if(ipc_call(mount.pid, &in, &out) == 0) {
		shm_id = proto_read_int(&out);
		if(size != NULL)
			*size = proto_read_int(&out);
	}
	proto_clear(&in);
	proto_clear(&out);
	return shm_id;
}

void flush(int fd) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) == 0) {
		proto_t in, out;
		proto_init(&in, NULL, 0);
		proto_init(&out, NULL, 0);

		proto_add_int(&in, FS_CMD_FLUSH);
		proto_add_int(&in, fd);
		proto_add(&in, &info, sizeof(fsinfo_t));
		ipc_call(mount.pid, &in, &out);
		proto_clear(&in);
		proto_clear(&out);
	}
}

int fcntl_raw(int fd, int cmd, proto_t* arg_in, proto_t* arg_out) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;

	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_CNTL);
	proto_add_int(&in, fd);
	proto_add(&in, &info, sizeof(fsinfo_t));
	proto_add_int(&in, cmd);
	if(arg_in == NULL)
		proto_add(&in, NULL, 0);
	else
		proto_add(&in, arg_in->data, arg_in->size);

	int res = -1;
	if(ipc_call(mount.pid, &in, &out) == 0) {
		res = proto_read_int(&out);
		if(arg_out != NULL) {
			int32_t sz;
			void *p = proto_read(&out, &sz);
			proto_copy(arg_out, p, sz);
		}
	}
	proto_clear(&in);
	proto_clear(&out);
	return res;
}
