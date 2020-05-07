#include <sys/vfs.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <mstr.h>
#include <sys/ipc.h>
#include <sys/kserv.h>
#include <sys/vfsc.h>

inline static int get_vfsd_pid(void) {
	return kserv_get(KSERV_VFS);
}

int vfs_new_node(fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_NEW_NODE, &in, &out);
	PF->clear(&in);
	if(res == 0)
		proto_read_to(&out, info, sizeof(fsinfo_t));
	PF->clear(&out);
	return res;	
}

const char* vfs_fullname(const char* fname) {
	str_t* fullname = str_new("");
	if(fname[0] == '/') {
		str_cpy(fullname, fname);
	}
	else {
		char pwd[FS_FULL_NAME_MAX];
		getcwd(pwd, FS_FULL_NAME_MAX-1);
		str_cpy(fullname, pwd);
		if(pwd[1] != 0)
			str_addc(fullname, '/');
		str_add(fullname, fname);
	}

	static char ret[FS_FULL_NAME_MAX];
	strncpy(ret, fullname->cstr, FS_FULL_NAME_MAX-1);
	str_free(fullname);
	return ret;
}

int vfs_open(fsinfo_t* info, int oflag, uint32_t* ufid) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t))->addi(&in, oflag);
	PF->init(&out, NULL, 0);

	int res = ipc_call(get_vfsd_pid(), VFS_OPEN, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(ufid != NULL)
			*ufid = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;	
}

int vfs_read_pipe(fsinfo_t* info, void* buf, uint32_t size, int block) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t))->addi(&in, size)->addi(&in, block);
	PF->init(&out, NULL, 0);

	int vfsd_pid = get_vfsd_pid();
	int res = ipc_call(vfsd_pid, VFS_PIPE_READ, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(res > 0)
			res = proto_read_to(&out, buf, size);
	}
	PF->clear(&out);

	if(res == 0 && block == 1) {//empty , do retry
		syscall2(SYS_PROC_BLOCK, vfsd_pid, info->node);
	}
	return res;	
}

int vfs_write_pipe(fsinfo_t* info, const void* buf, uint32_t size, int block) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t))->add(&in, buf, size)->addi(&in, block);
	PF->init(&out, NULL, 0);

	int res = ipc_call(get_vfsd_pid(), VFS_PIPE_WRITE, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;	
}

int vfs_dup(int fd) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, fd);
	PF->init(&out, NULL, 0);

	int res = ipc_call(get_vfsd_pid(), VFS_DUP, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_close(int fd) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, fd);
	PF->init(&out, NULL, 0);

	int res = ipc_call(get_vfsd_pid(), VFS_CLOSE, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_dup2(int fd, int to) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, fd)->addi(&in, to);
	PF->init(&out, NULL, 0);

	int res = ipc_call(get_vfsd_pid(), VFS_DUP2, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_open_pipe(int fd[2]) {
	proto_t out;
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_PIPE_OPEN, NULL, &out);
	if(res == 0) {
		if(proto_read_int(&out) == 0) {
			fd[0] = proto_read_int(&out);
			fd[1] = proto_read_int(&out);
		}
		else {
			res = -1;
		}
	}
	PF->clear(&out);
	return res;
}

int vfs_get(const char* fname, fsinfo_t* info) {
	fname = vfs_fullname(fname);
	proto_t in, out;
	PF->init(&in, NULL, 0)->adds(&in, fname);
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_BY_NAME, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out); //res = node
		if(res != 0) {
			proto_read_to(&out, info, sizeof(fsinfo_t));
			res = 0;
		}
		else
			res = -1;
	}
	PF->clear(&out);
	return res;	
}

int  vfs_access(const char* fname) {
	fsinfo_t info;
	return vfs_get(fname, &info);
}

fsinfo_t* vfs_kids(fsinfo_t* info, uint32_t *num) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_KIDS, &in, &out);
	PF->clear(&in);

	fsinfo_t* ret = NULL;
	if(res == 0) {
		uint32_t n = proto_read_int(&out);
		*num = n;
		if(n > 0) {
			ret = (fsinfo_t*)malloc(n * sizeof(fsinfo_t));
			proto_read_to(&out, ret, n * sizeof(fsinfo_t));
		}
	}
	PF->clear(&out);
	return ret;
}

int vfs_set(fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_SET_FSINFO, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_add(fsinfo_t* to, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->
		add(&in, to, sizeof(fsinfo_t))->
		add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_ADD, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_del(fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_DEL, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}
/*
int vfs_get_mount(fsinfo_t* info, mount_t* mount) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_MOUNT, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		proto_read_to(&out, mount, sizeof(mount_t));
	}
	PF->clear(&out);
	return res;
}
*/

int vfs_get_mount_by_id(int id, mount_t* mount) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, id);
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_MOUNT_BY_ID, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
		if(res == 0)
			proto_read_to(&out, mount, sizeof(mount_t));
	}
	PF->clear(&out);
	return res;
}

int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->
		add(&in, mount_to, sizeof(fsinfo_t))->
		add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_MOUNT, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_umount(fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_UMOUNT, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_get_by_fd(int fd, uint32_t *ufid, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, fd);
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_BY_FD, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		int uid = proto_read_int(&out);
		if(ufid != NULL)
			*ufid = uid;

		if(uid == 0) {
			res = -1;
		}
		else {
			if(info != NULL)
				proto_read_to(&out, info, sizeof(fsinfo_t));
		}
	}
	PF->clear(&out);
	return res;
}

int vfs_check_fd(int pid, int fd) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, fd)->addi(&in, pid);
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_CHECK_FD, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	else {
		res = 0;
	}
	PF->clear(&out);
	return res;
}

int vfs_tell(int fd) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, fd);
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_TELL, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_seek(int fd, int offset) {
	proto_t in, out;
	PF->init(&in, NULL, 0)->addi(&in, fd)->addi(&in, offset);
	PF->init(&out, NULL, 0);
	int res = ipc_call(get_vfsd_pid(), VFS_SEEK, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

void* vfs_readfile(const char* fname, int* rsz) {
	fname = vfs_fullname(fname);
	fsinfo_t info;
	if(vfs_get(fname, &info) != 0 || info.size <= 0)
		return NULL;
	void* buf = malloc(info.size);
	if(buf == NULL)
		return NULL;
	char* p = (char*)buf;
	int fd = open(fname, O_RDONLY);
	int fsize = info.size;
	if(fd >= 0) {
		while(1) {
			int sz = read(fd, p, fsize);
			if(sz <= 0 && errno != EAGAIN)
				break;
			if(sz > 0) {
				fsize -= sz;
				p += sz;
				if(fsize <= 0)
					break;
			}
		}
		close(fd);
	}

	if(fsize != 0) {
		free(buf);
		return NULL;
	}

	if(rsz != NULL)
		*rsz = info.size;
	return buf;
}

int vfs_create(const char* fname, fsinfo_t* ret, int type) {
	str_t *dir = str_new("");
	str_t *name = str_new("");
	vfs_parse_name(fname, dir, name);

	fsinfo_t info_to;
	if(vfs_get(CS(dir), &info_to) != 0) {
		str_free(dir);
		str_free(name);
		return -1;
	}
	
	/*mount_t mount;
	if(vfs_get_mount(&info_to, &mount) != 0) {
		str_free(dir);
		str_free(name);
		return -1;
	}
	*/

	memset(ret, 0, sizeof(fsinfo_t));
	strcpy(ret->name, CS(name));
	ret->type = type;
	str_free(name);
	str_free(dir);

	vfs_new_node(ret);
	if(vfs_add(&info_to, ret) != 0) {
		vfs_del(ret);
		return -1;
	}
	if(type == FS_TYPE_CHAR || type == FS_TYPE_BLOCK)
		return 0;
	
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		add(&in, &info_to, sizeof(fsinfo_t))->
		add(&in, ret, sizeof(fsinfo_t));

	int res = -1;
	if(ipc_call(info_to.mount_pid, FS_CMD_CREATE, &in, &out) != 0) {
		vfs_del(ret);
	}
	else {
		res = proto_read_int(&out);
		if(res == 0) {
			proto_read_to(&out, ret, sizeof(fsinfo_t));
			res = vfs_set(ret);
		}
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int vfs_parse_name(const char* fname, str_t* dir, str_t* name) {
	str_t* fullstr = str_new(vfs_fullname(fname));
	char* full = (char*)CS(fullstr);
	int i = strlen(full);
	while(i >= 0) {
		if(full[i] == '/') {
			full[i] = 0;
			break;
		}
		--i;	
	}
	str_cpy(dir, full);
	str_cpy(name, full+i+1);
	if(CS(dir)[0] == 0)
		str_cpy(dir, "/");
	str_free(fullstr);
	return 0;
}

/*
int vfs_seek(int fd, int offset, int whence) {
	return syscall3(SYS_VFS_PROC_SEEK, (int32_t)fd,(int32_t)offset, (int32_t)whence);
}
*/

