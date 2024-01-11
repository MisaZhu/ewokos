#include <ewoksys/vfs.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/proc.h>
#include <sys/shm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int fd;
	uint32_t offset;
	fsinfo_t info;
} fsinfo_buffer_t;

#define MAX_FINFO_BUFFER  32 
static fsinfo_buffer_t _fsinfo_buffers[MAX_FINFO_BUFFER];

void  vfs_init(void) {
	for(uint32_t i=0; i<MAX_FINFO_BUFFER; i++) {
		memset(&_fsinfo_buffers[i], 0, sizeof(fsinfo_buffer_t));
		_fsinfo_buffers[i].fd = -1;
	}
}

static inline void vfs_set_info_buffer(int fd, fsinfo_t* info) {
	for(uint32_t i=0; i<MAX_FINFO_BUFFER; i++) {
		if(_fsinfo_buffers[i].fd < 0) {
			_fsinfo_buffers[i].fd = fd;
			memcpy(&_fsinfo_buffers[i].info, info, sizeof(fsinfo_t));
			return;
		}
	}
}

static inline void vfs_clear_info_buffer(int fd) {
	for(uint32_t i=0; i<MAX_FINFO_BUFFER; i++) {
		if(_fsinfo_buffers[i].fd == fd) {
			memset(&_fsinfo_buffers[i], 0, sizeof(fsinfo_buffer_t));
			_fsinfo_buffers[i].fd = -1;
			return;
		}
	}
}

static inline fsinfo_buffer_t* vfs_fetch_info_buffer(int fd) {
	for(uint32_t i=0; i<MAX_FINFO_BUFFER; i++) {
		if(_fsinfo_buffers[i].fd == fd)
			return &_fsinfo_buffers[i];
	}
	return NULL;
}

int vfs_check_access(int pid, fsinfo_t* info, int mode) {
	procinfo_t procinfo;
	if(info == NULL || proc_info(pid, &procinfo) != 0)
    return -1;

	if(procinfo.uid <= 0)
		return 0;

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

int vfs_get_by_fd(int fd, fsinfo_t* info) {
	fsinfo_buffer_t* buffer = vfs_fetch_info_buffer(fd);
	if(buffer != NULL) {
		memcpy(info, &buffer->info, sizeof(fsinfo_t));
		return 0;
	}
	
	int res = vfs_get_by_fd_raw(fd, info);
	if(res == 0 && fd > 3)
		vfs_set_info_buffer(fd, info);
	return res;
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

int vfs_new_node(fsinfo_t* info, uint32_t node_to) {
	proto_t in, out;
	PF->init(&in)->add(&in, info, sizeof(fsinfo_t))->addi(&in, node_to);
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

	static char ret[FS_FULL_NAME_MAX];
	sstrncpy(ret, fullname->cstr, FS_FULL_NAME_MAX-1);
	str_free(fullname);
	return ret;
}

int vfs_open(fsinfo_t* info, int oflag) {
	proto_t in, out;
	PF->init(&in)->add(&in, info, sizeof(fsinfo_t))->addi(&in, oflag);
	PF->init(&out);

	int res = ipc_call(get_vfsd_pid(), VFS_OPEN, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(res >= 0) {
			proto_read_to(&out, info, sizeof(fsinfo_t));
			vfs_clear_info_buffer(res);	
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
	PF->init(&in)->addi(&in, fd)->addi(&in, node)->addi(&in, size)->addi(&in, block?1:0);
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
	PF->init(&in)->addi(&in, fd)->addi(&in, node)->add(&in, buf, size)->addi(&in, block?1:0);
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

int vfs_dup(int fd) {
	proto_t in, out;
	PF->init(&in)->addi(&in, fd);
	PF->init(&out);

	int res = ipc_call(get_vfsd_pid(), VFS_DUP, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_close(int fd) {
	vfs_clear_info_buffer(fd);	
	proto_t in;
	PF->init(&in)->addi(&in, fd);
	int res = ipc_call_wait(get_vfsd_pid(), VFS_CLOSE, &in);
	PF->clear(&in);
	return res;
}

int vfs_dup2(int fd, int to) {
	proto_t in, out;
	PF->init(&in)->addi(&in, fd)->addi(&in, to);
	PF->init(&out);

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
	PF->init(&out);
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

int  vfs_access(const char* fname) {
	return vfs_get_by_name(fname, NULL);
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

int vfs_set(fsinfo_t* info) {
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
	fsinfo_buffer_t* buffer = vfs_fetch_info_buffer(fd);
	if(buffer != NULL)
		return buffer->offset;

	proto_t in, out;
	PF->init(&in)->addi(&in, fd);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_TELL, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int vfs_seek(int fd, int offset) {
	fsinfo_buffer_t* buffer = vfs_fetch_info_buffer(fd);
	if(buffer != NULL) {
		buffer->offset = offset;
		return 0;
	}

	proto_t in, out;
	PF->init(&in)->addi(&in, fd)->addi(&in, offset);
	PF->init(&out);
	int res = ipc_call(get_vfsd_pid(), VFS_SEEK, &in, &out);
	PF->clear(&in);

	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	if(res >= 0)
		return 0;
	return -1;
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
	str_t *dir = str_new("");
	str_t *name = str_new("");
	vfs_parse_name(fname, dir, name);

	fsinfo_t info_to;
	if(vfs_get_by_name(CS(dir), &info_to) != 0) {
		int res_dir = -1;
		if(autodir)
			res_dir = vfs_create(CS(dir), &info_to, FS_TYPE_DIR, 0755, true, autodir);
		if(res_dir != 0) {
			str_free(dir);
			str_free(name);
			return -1;
		}
	}

	fsinfo_t fi;
	memset(&fi, 0, sizeof(fsinfo_t));
	strcpy(fi.name, CS(name));
	fi.type = type;
	str_free(name);
	str_free(dir);
	if(type == FS_TYPE_DIR)
		fi.stat.size = 1024;

	fi.stat.uid = getuid();
	fi.stat.gid = getgid();
	fi.stat.mode = mode;

	if(vfs_new_node(&fi, info_to.node) != 0)
		return -1;

	if(vfs_node_only) {//only create in vfs service, not existed in storage. 
		if(ret != NULL)
			memcpy(ret, &fi, sizeof(fsinfo_t));
		return 0;
	}

	fi.mount_pid = info_to.mount_pid;
	int res = dev_create(info_to.mount_pid, &info_to, &fi);
	if(res != 0)
		vfs_del_node(fi.node);
	
	if(ret != NULL)
		memcpy(ret, &fi, sizeof(fsinfo_t));
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

int vfs_fcntl(int fd, int cmd, proto_t* arg_in, proto_t* arg_out) {
	fsinfo_t info;
	//if(vfs_get_by_fd(fd, &info) != 0)
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
	
	proto_t in;
	PF->init(&in)->
		addi(&in, fd)->
		addi(&in, info.node)->
		addi(&in, cmd);
	if(arg_in == NULL)
		PF->add(&in, NULL, 0);
	else
		PF->add(&in, arg_in->data, arg_in->size);

	int res = -1;
	if(arg_out != NULL) {
		proto_t out;
		PF->init(&out);
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
	}
	else {
		res = ipc_call(info.mount_pid, FS_CMD_CNTL, &in, NULL);
		PF->clear(&in);
	}

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

#define SHM_ON 128
int vfs_read(int fd, fsinfo_t *info, void* buf, uint32_t size) {
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
	else if(res == ERR_RETRY) {
		errno = EAGAIN;
		res = -1;
	}
	else if(res == ERR_RETRY_NON_BLOCK) {
		errno = EAGAIN_NON_BLOCK;
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
		if(info->type == FS_TYPE_FILE)
			vfs_seek(fd, offset);
	}
	else if(res == -2) {
		errno = EAGAIN;
		res = -1;
	}
	return res;
}

#ifdef __cplusplus
}
#endif

