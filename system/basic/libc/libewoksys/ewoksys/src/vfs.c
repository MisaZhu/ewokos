#include <ewoksys/vfs.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/proc.h>
#include <sys/shm.h>
// #include <sys/unistd.h>
#ifdef __cplusplus
extern "C" {
#endif


static fsfile_t _fsfiles[MAX_OPEN_FILE_PER_PROC];

static int vfs_get_by_fd_raw(int fd, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in)->addi(&in, fd);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_BY_FD, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out); //res = node
		if(res != 0) {
			if(info != NULL)
				proto_read_to(&out, info, sizeof(fsinfo_t));
			res = 0;
		}
		else
			res = -1;
	}
	PF->clear(&out);
	return res;
}

static int vfs_set_info(fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_SET_FSINFO, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

void  vfs_init(void) {
	for(uint32_t i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		memset(&_fsfiles[i], 0, sizeof(fsfile_t));
	}
}

static inline fsfile_t* vfs_set_file(int fd, fsinfo_t* info) {
	if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
		return NULL;
	fsfile_t* file = &_fsfiles[fd];
	memcpy(&file->info, info, sizeof(fsinfo_t));
	return file;
}

static inline void vfs_update_file(fsinfo_t* info) {
	for(uint32_t i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
		if(_fsfiles[i].info.node == info->node)
			memcpy(&_fsfiles[i].info, info, sizeof(fsinfo_t));
	}
}

static inline void vfs_clear_file(int fd) {
	if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
		return;
	memset(&_fsfiles[fd], 0, sizeof(fsfile_t));
}

static fsfile_t* vfs_get_file(int fd) {
	if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
		return NULL;

	fsfile_t* ret = &_fsfiles[fd];
	if(ret->info.node != 0)
		return ret;

	fsinfo_t info;
	if(vfs_get_by_fd_raw(fd, &info) != 0)
		return NULL;
	return vfs_set_file(fd, &info);
}

int vfs_get_flags(int fd) {
	fsfile_t* file = vfs_get_file(fd);
	if(file == NULL)
		return -1;
	return file->flags;
}

int vfs_set_flags(int fd, int flags) {
	fsfile_t* file = vfs_get_file(fd);
	if(file == NULL)
		return -1;
	file->flags = flags;
	return 0;
}

int vfs_check_access(int pid, fsinfo_t* info, int mode) {
	procinfo_t procinfo;
	if(info == NULL || proc_info(pid, &procinfo) != 0)
    return -1;

	if(procinfo.uid <= 0) {
		if(mode == X_OK && (info->stat.mode & 0111) == 0)
			return -1;
		return 0;
	}

	int ucheck = 0400;	
	int gcheck = 040;	
	int acheck = 04;	
	if(mode == R_OK) {
		ucheck = 0400;	
		gcheck = 040;	
		acheck = 04;	
	}
	else if(mode == W_OK) {
		ucheck = 0200;	
		gcheck = 020;	
		acheck = 02;	
	}
	else if(mode == X_OK) {
		ucheck = 0100;	
		gcheck = 010;	
		acheck = 01;	
	}

	if(procinfo.uid == info->stat.uid) {
		if((info->stat.mode & ucheck) != 0)
			return 0;
	}
	else if(procinfo.gid == info->stat.gid) {
		if((info->stat.mode & gcheck) != 0)
			return 0;
	}
	else {
		if((info->stat.mode & acheck) != 0)
			return 0;
	}
	return -1;
}

int vfs_get_by_fd(int fd, fsinfo_t* info) {
	fsfile_t* buffer = vfs_get_file(fd);
	if(buffer != NULL) {
		memcpy(info, &buffer->info, sizeof(fsinfo_t));
		return 0;
	}
	return -1;
}

int vfs_get_by_node(uint32_t node, fsinfo_t* info) {
	proto_t in, out;
	PF->init(&in)->addi(&in, node);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_BY_NODE, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out); //res = node
		if(res != 0) {
			if(info != NULL)
				proto_read_to(&out, info, sizeof(fsinfo_t));
			res = 0;
		}
		else
			res = -1;
	}
	PF->clear(&out);
	return res;
}

int vfs_new_node(fsinfo_t* info, uint32_t node_to, bool vfs_node_only) {
	proto_t in, out;
	PF->format(&in, "m,i,i", info, sizeof(fsinfo_t), node_to, vfs_node_only);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_NEW_NODE, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out); //res = node
		if(res == 0)
			proto_read_to(&out, info, sizeof(fsinfo_t));
		else
			res = -1;
	}
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

	static char ret[FS_FULL_NAME_MAX] = {0};
	strncpy(ret, fullname->cstr, FS_FULL_NAME_MAX-1);
	str_free(fullname);
	return ret;
}

int vfs_open(fsinfo_t* info, int oflag) {
	proto_t in, out;
	PF->format(&in, "m,i", info, sizeof(fsinfo_t), oflag);
	//PF->init(&in)->add(&in, info, sizeof(fsinfo_t))->addi(&in, oflag);
	PF->init(&out);

	int res = ipc_call(get_vfsd_pid(), VFS_OPEN, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(res >= 0) {
			proto_read_to(&out, info, sizeof(fsinfo_t));
			fsfile_t* file = vfs_set_file(res, info);	
			if(file != NULL)
				file->flags = oflag;
		}
		else {
			errno = proto_read_int(&out);
		}
	}
	PF->clear(&out);
	return res;	
}

static int read_pipe(int fd, uint32_t node, void* buf, uint32_t size, bool block) {
	proto_t in, out;
	PF->format(&in, "i,i,i,i", fd, node, size, block?1:0);
	PF->init(&out);

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
		proc_block_by(vfsd_pid, node);
	}
	return res;	
}

static int write_pipe(int fd, uint32_t node, const void* buf, uint32_t size, bool block) {
	proto_t in, out;
	//PF->init(&in)->addi(&in, fd)->addi(&in, node)->add(&in, buf, size)->addi(&in, block?1:0);
	PF->format(&in, "i,i,m,i", fd, node, buf, size, block?1:0);
	PF->init(&out);

	int vfsd_pid = get_vfsd_pid();
	int res = ipc_call(vfsd_pid, VFS_PIPE_WRITE, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);

	if(res == 0 && block == 1) {//empty , do retry
		proc_block_by(vfsd_pid, node); 
	}
	return res;	
}

int vfs_close_info(int fd) {
	proto_t in;
	PF->init(&in)->addi(&in, fd);
	int res = ipc_call(get_vfsd_pid(), VFS_CLOSE, &in, NULL);
	PF->clear(&in);
	return res;
}

int vfs_close(int fd) {
	fsfile_t* file = vfs_get_file(fd);
	if(file == NULL)
		return -1;
	if((file->info.state & FS_STATE_CHANGED) != 0) {
		if(vfs_update(&file->info, true) != 0)
			return -1;
	}

	proto_t in;
	PF->format(&in, "i,i,m",
		fd, file->info.node, &file->info, sizeof(fsinfo_t));
	int res = ipc_call(file->info.mount_pid, FS_CMD_CLOSE, &in, NULL);	
	PF->clear(&in);

	vfs_clear_file(fd);	
	return vfs_close_info(fd);
}

int vfs_dup(int fd) {
	fsfile_t* file = vfs_get_file(fd);
	if(file == NULL)
		return -1;

	proto_t in, out;
	PF->init(&in)->addi(&in, fd);
	PF->init(&out);

	int res = ipc_call(get_vfsd_pid(), VFS_DUP, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(res >= 0) {
			fsinfo_t info;
			proto_read_to(&out, &info, sizeof(fsinfo_t));
			fsfile_t* file_to = vfs_set_file(res, &info);
			file_to->flags = file->flags;
		}
	}
	PF->clear(&out);
	return res;
}

int vfs_dup2(int fd, int to) {
	fsfile_t* file = vfs_get_file(fd);
	if(file == NULL)
		return -1;
	vfs_clear_file(to);	

	proto_t in, out;
	PF->format(&in, "i,i", fd, to);
	PF->init(&out);

	int res = ipc_call(get_vfsd_pid(), VFS_DUP2, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(res >= 0) {
			fsinfo_t info;
			proto_read_to(&out, &info, sizeof(fsinfo_t));
			fsfile_t* file_to = vfs_set_file(res, &info);
			file_to->flags = file->flags;
		}
	}
	PF->clear(&out);
	return res;
}

int vfs_open_pipe(int fd[2]) {
	proto_t out;
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_PIPE_OPEN, NULL, &out);
	if(res == 0) {
		if(proto_read_int(&out) == 0) {
			fd[0] = proto_read_int(&out);
			fd[1] = proto_read_int(&out);
			fsinfo_t info;
			proto_read_to(&out, &info, sizeof(fsinfo_t));
			vfs_set_file(fd[0], &info);
			vfs_set_file(fd[1], &info);
		}
		else {
			res = -1;
		}
	}
	PF->clear(&out);
	return res;
}

int vfs_get_by_name(const char* fname, fsinfo_t* info) {
	fname = vfs_fullname(fname);
	proto_t in, out;
	PF->init(&in)->adds(&in, fname);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_GET_BY_NAME, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out); //res = node
		if(res != 0) {
			if(info != NULL)
				proto_read_to(&out, info, sizeof(fsinfo_t));
			res = 0;
		}
		else
			res = -1;
	}
	PF->clear(&out);
	return res;	
}

fsinfo_t* vfs_kids(uint32_t node, uint32_t *num) {
	proto_t in, out;
	PF->init(&in)->addi(&in, node);
	PF->init(&out);
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

int vfs_update(fsinfo_t* info, bool do_dev) {
	if(do_dev) {
		if(dev_set(info->mount_pid, info) != 0)
			return -1;
	}

	if(vfs_set_info(info) != 0)
		return -1;
	vfs_update_file(info);
	return 0;
}

int vfs_del_node(uint32_t node) {
	proto_t in, out;
	PF->init(&in)->addi(&in, node);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_DEL_NODE, &in, &out);
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
	PF->init(&in)->add(&in, info, sizeof(fsinfo_t));
	PF->init(&out);
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
	PF->init(&in)->addi(&in, id);
	PF->init(&out);
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

int vfs_mount(uint32_t mount_node_to, uint32_t node) {
	proto_t in, out;
	PF->init(&in)->
		addi(&in, mount_node_to)->
		addi(&in, node);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_MOUNT, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_umount(uint32_t node) {
	proto_t in, out;
	PF->init(&in)->addi(&in, node);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_UMOUNT, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_tell(int fd) {
	fsfile_t* buffer = vfs_get_file(fd);
	if(buffer == NULL)
		return 0;
	return buffer->offset;
}

int vfs_seek(int fd, int offset) {
	fsfile_t* buffer = vfs_get_file(fd);
	if(buffer == NULL)
		return -1;
	
	buffer->offset = offset;
	return 0;
}

void* vfs_readfile(const char* fname, int* rsz) {
	fname = vfs_fullname(fname);
	fsinfo_t info;
	if(vfs_get_by_name(fname, &info) != 0 || info.stat.size <= 0)
		return NULL;
	void* buf = malloc(info.stat.size+1); //one more char for string end.
	if(buf == NULL)
		return NULL;

	char* p = (char*)buf;
	int fd = open(fname, O_RDONLY);
	int fsize = info.stat.size;
	if(fd >= 0) {
		while(fsize > 0) {
			int sz = read(fd, p, VFS_BUF_SIZE < fsize ? VFS_BUF_SIZE:fsize);
			if(sz < 0 && errno != EAGAIN)
				break;
			if(sz > 0) {
				fsize -= sz;
				p += sz;
			}
		}
		close(fd);
	}

	if(fsize != 0) {
		free(buf);
		return NULL;
	}

	if(rsz != NULL)
		*rsz = info.stat.size;
	return buf;
}

int vfs_create(const char* fname, fsinfo_t* ret, int type, int mode, bool vfs_node_only, bool autodir) {
	char dir[FS_FULL_NAME_MAX];
	char name[FS_FULL_NAME_MAX];
	vfs_dir_name(fname, dir, FS_FULL_NAME_MAX);
	vfs_file_name(fname, name, FS_FULL_NAME_MAX);

	fsinfo_t info_to;
	if(vfs_get_by_name(dir, &info_to) != 0) {
		int res_dir = -1;
		if(autodir)
			res_dir = vfs_create(dir, &info_to, FS_TYPE_DIR, 0755, vfs_node_only, autodir);
		if(res_dir != 0)
			return -1;
	}

	if(name == NULL || name[0] == 0)
		return 0;

	fsinfo_t fi;
	memset(&fi, 0, sizeof(fsinfo_t));
	strcpy(fi.name, name);
	fi.type = type;
	if(type == FS_TYPE_DIR)
		fi.stat.size = 1024;

	fi.stat.uid = getuid();
	fi.stat.gid = getgid();
	fi.stat.mode = mode;

	if(vfs_new_node(&fi, info_to.node, vfs_node_only) != 0)
		return -1;

	if(vfs_node_only) {//only create in vfs service, not existed in storage. 
		if(ret != NULL)
			memcpy(ret, &fi, sizeof(fsinfo_t));
		return 0;
	}

	fi.mount_pid = info_to.mount_pid;
	int res = dev_create(info_to.mount_pid, &info_to, &fi);
	if(res == 0) 
		vfs_set_info(&fi);

	if(res != 0) {
		vfs_del_node(fi.node);
		return res;
	}

	if(ret != NULL)
		memcpy(ret, &fi, sizeof(fsinfo_t));
	return res;
}

const char* vfs_dir_name(const char* fname, char* ret, uint32_t len) {
	memset(ret, 0, len);
	strncpy(ret, fname, len-1);
	int i = strlen(ret)-1;
	while(i >= 0) {
		if(ret[i] == '/') {
			ret[i] = 0;
			break;
		}
		--i;	
	}

	if(ret[0] == 0)
		strcpy(ret, "/");
	return ret;
}

const char* vfs_file_name(const char* fname, char* ret, uint32_t len) {
	memset(ret, 0, len);
	int i = strlen(fname)-1;
	while(i >= 0) {
		if(fname[i] == '/') {
			break;
		}
		--i;	
	}
	++i;
	strncpy(ret, fname+i, len-1);
	return ret;
}

int vfs_fcntl(int fd, int cmd, proto_t* arg_in, proto_t* arg_out) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	int res = dev_fcntl(info.mount_pid, fd, &info, cmd, arg_in, arg_out);
	if(res == 0)
		vfs_update_file(&info);
	return res;
}

inline int  vfs_fcntl_wait(int fd, int cmd, proto_t* in) {
	proto_t out;
	PF->init(&out);
	int res = vfs_fcntl(fd, cmd, in, &out);
	PF->clear(&out);
	return res;
}

int32_t vfs_dma(int fd, int* size) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return 0;
	return dev_dma(info.mount_pid, fd, info.node, size);
}

int vfs_flush(int fd, bool wait) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return 0; //error

	return dev_flush(info.mount_pid, fd, info.node, wait);
}

int vfs_read_pipe(int fd, uint32_t node, void* buf, uint32_t size, bool block) {
	int res = read_pipe(fd, node, buf, size, block);
	if(res == 0) { // pipe empty, do retry
		errno = EAGAIN;
		return -1;
	}
	if(res > 0) {
		return res;
	}
	return 0; //res < 0 , pipe closed, return 0.
}

int vfs_read(int fd, fsinfo_t *info, void* buf, uint32_t size) {
	errno = 0;
	int offset = 0;
	if(info->type == FS_TYPE_FILE) {
		offset = vfs_tell(fd);
		if(offset < 0)
			offset = 0;
	}

	int res = dev_read(info->mount_pid, fd, info, offset, buf, size);
	if(res > 0) {
		offset += res;
		if(info->type == FS_TYPE_FILE)
			vfs_seek(fd, offset);
	}
	else if(res == VFS_ERR_RETRY) {
		errno = EAGAIN;
		res = -1;
	}
	return res;
}

int vfs_write_pipe(int fd, uint32_t node, const void* buf, uint32_t size, bool block) {
	int res = write_pipe(fd, node, buf, size, block);
	if(res == 0) { // pipe not empty, do retry
		errno = EAGAIN;
		return -1;
	}
	if(res > 0)
		return res;
	return 0; //res < 0 , pipe closed, return 0.
}

int vfs_write(int fd, fsinfo_t* info, const void* buf, uint32_t size) {
	if(info->type == FS_TYPE_DIR) 
		return -1;

	int offset = 0;
	if(info->type == FS_TYPE_FILE) {
		offset = vfs_tell(fd);
		if(offset < 0)
			offset = 0;
	}
		
	int res = dev_write(info->mount_pid, fd, info, offset, buf, size);
	if(res > 0) {
		offset += res;
		if(info->type == FS_TYPE_FILE) {
			vfs_update_file(info);
			vfs_seek(fd, offset);
		}
	}
	else if(res == VFS_ERR_RETRY) {
		errno = EAGAIN;
		res = -1;
	}
	return res;
}

#ifdef __cplusplus
}
#endif

