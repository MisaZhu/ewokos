#include <kserv.h>
#include <vfs/fs.h>
#include <vprintf.h>
#include <vfs/vfs.h>
#include <ipc.h>
#include <kstring.h>
#include <syscall.h>
#include <stdlib.h>
#include <proto.h>
#include <unistd.h>

#define FS_BUF_SIZE (16*KB)

int fs_open(const char* name, int32_t flags) {
	int fd = vfs_open(name, flags);
	if(fd < 0)
		return -1;

	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0) {
		fs_close(fd);
		return -1;
	}
	if(dev_serv_pid == 0) 
		return fd;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, fd);
	proto_add_int(in, flags);
	int32_t res = ipc_call(dev_serv_pid, FS_OPEN, in, out);
	proto_free(in);
	if(res != 0) {
		fs_close(fd);
		fd = -1;
	}
	proto_free(out);
	return fd;
}

inline int32_t fs_update(fs_info_t* info) {
	return syscall1(SYSCALL_PFILE_INFO_UPDATE, (int32_t)info);
}

int fs_pipe_open(int fds[2]) {
	return vfs_pipe_open(fds);
}

int fs_close(int fd) {
	return vfs_close(fd);
}

int fs_remove(const char* fname) {
	return vfs_del(fname);
}

int32_t fs_dma(int fd, uint32_t* size) {
	if(size != NULL)
		*size = 0;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid <= 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, fd);
	int32_t res = ipc_call(dev_serv_pid, FS_DMA, in, out);
	proto_free(in);

	int32_t shm_id = -1;
	if(res == 0) {
		shm_id = proto_read_int(out);
		if(size != NULL)
			*size = proto_read_int(out);
	}
	proto_free(out);
	return shm_id;
}

int32_t fs_flush(int fd) {
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;
	
	if(dev_serv_pid > 0) {
		proto_t* in = proto_new(NULL, 0);
		proto_add_int(in, fd);
		int32_t res = ipc_call(dev_serv_pid, FS_FLUSH, in, NULL);
		proto_free(in);
		return res;
	}
	return 0;
}

int fs_read(int fd, char* buf, uint32_t size) {
	if(fd < 0)
		return -1;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;
	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, fd);
	proto_add_int(in, size);
	proto_add_int(in, seek);

	int res = ipc_call(dev_serv_pid, FS_READ, in, out);
	proto_free(in);
	if(res < 0) {
		proto_free(out);
		return -1;
	}

	int32_t sz = proto_read_int(out);
	void *p = NULL;
	if(sz > 0)
	 	p = proto_read(out, NULL);
	if(p != NULL && sz > 0)
		memcpy(buf, p, sz);
	proto_free(out);

	seek += sz;
	syscall2(SYSCALL_PFILE_SEEK, fd, seek);
	return sz;
}

int fs_fctrl(const char* fname, int32_t cmd, const proto_t* input, proto_t* output) {
	if(fname[0] == 0)
		return -1;
	fs_info_t info;
	if(fs_finfo(fname, &info) != 0)
		return -1;
	if(info.dev_serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add_str(in, fname);
	proto_add_int(in, cmd);
	if(input != NULL)
		proto_add(in, input->data, input->size);
	int32_t res = ipc_call(info.dev_serv_pid, FS_FCTRL, in, output);
	proto_free(in);
	return res;
}

int fs_ctrl(int fd, int32_t cmd, const proto_t* input, proto_t* output) {
	if(fd < 0)
		return -1;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_add_int(in, fd);
	proto_add_int(in, cmd);
	if(input != NULL)
		proto_add(in, input->data, input->size);
	int32_t res = ipc_call(dev_serv_pid, FS_CTRL, in, output);
	proto_free(in);
	return res;
}

int fs_write(int fd, const char* buf, uint32_t size) {
	if(fd < 0)
		return -1;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;

	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* in = proto_new(NULL, 0);
	proto_t* out = proto_new(NULL, 0);
	proto_add_int(in, fd);
	proto_add(in, (void*)buf, size);
	proto_add_int(in, seek);

	int32_t res = ipc_call(dev_serv_pid, FS_WRITE, in, out);
	proto_free(in);
	int sz = -1;
	if(res == 0) {
		sz = proto_read_int(out);
		seek += sz;
		syscall2(SYSCALL_PFILE_SEEK, fd, seek);
	}
	return sz;
}

int32_t fs_add(const char* dir_name, const char* name, uint32_t type) {
	if(dir_name[0] == 0 || strlen(name) == 0)
		return -1;

	fs_info_t info;
	if(fs_finfo(dir_name, &info) != 0)
		return -1;

	int32_t res;
	if(type == FS_TYPE_DIR)
		res = vfs_add(dir_name, name, VFS_DIR_SIZE, NULL);
	else
		res = vfs_add(dir_name, name, 0, NULL);
	if(res < 0)
		return -1;

	int32_t ret = -1;
	if(info.dev_serv_pid > 0) {
		proto_t* proto = proto_new(NULL, 0);
		tstr_t* fname = fs_make_fname(dir_name, name);
		proto_add_str(proto, CS(fname));
		int32_t res = ipc_call(info.dev_serv_pid, FS_ADD, proto, NULL);
		proto_free(proto);
		if(res != 0) {
			vfs_del(CS(fname));
			ret = -1;
		}
		else {
			ret = 0;
		}
		tstr_free(fname);
	}
	return ret;
}

int fs_getch(int fd) {
	char buf[1];
	if(fs_read(fd, buf, 1) != 1)
		return 0;
	return buf[0];
}

int fs_putch(int fd, int c) {
	char buf[1];
	buf[0] = (char)c;
	return fs_write(fd, buf, 1);
}

int fs_finfo(const char* name, fs_info_t* info) {
	if(name[0] == 0)
		return -1;
	return vfs_info_by_name(name, info);
}

tstr_t* fs_kid(const char* dir_name, int32_t index) {
	return vfs_kid(dir_name, index);
}

char* fs_read_file(const char* fname, int32_t *size) {
	fs_info_t info;
	if(fs_finfo(fname, &info) != 0 || info.size <= 0) {
		return NULL;
	}

	int fd = open(fname, O_RDONLY);
	if(fd < 0) 
		return NULL;

	int res = 0;
	int sz = info.size;
	char* buf = (char*)malloc(sz);
	char* p = buf;
	while(sz > 0) {
		res = read(fd, p, sz);
		if(res <= 0)
			break;
		sz -= res;
		p += res;
	}

	fs_close(fd);
	if(sz > 0 ) {
		free(buf);
		buf = NULL;
		*size = 0;
	}
	*size = info.size;
	return buf;
}

tstr_t* fs_full_name(const char* fname) {
	char pwd[FULL_NAME_MAX];
	getcwd(pwd, FULL_NAME_MAX-1);

	tstr_t* full = tstr_new("", MFS);
	if(fname[0] == 0) {
		tstr_cpy(full, pwd);
	}
	else if(fname[0] == '/') {
		tstr_cpy(full, fname);
	}
	else {
		if(strcmp(pwd, "/") == 0) {
			tstr_cpy(full, "/");
		}
		else {
			tstr_cpy(full, pwd);
			tstr_addc(full, '/');
		}
		tstr_add(full, fname);
	}
	return full;
}

int32_t fs_parse_name(const char* fname, tstr_t* dir, tstr_t* name) {
	tstr_t* fullstr = fs_full_name(fname);
	char* full = (char*)CS(fullstr);
	int32_t i = strlen(full);
	while(i >= 0) {
		if(full[i] == '/') {
			full[i] = 0;
			break;
		}
		--i;	
	}
	tstr_cpy(dir, full);
	tstr_cpy(name, full+i+1);
	if(CS(dir)[0] == 0)
		tstr_cpy(dir, "/");
	tstr_free(fullstr);
	return 0;
}

tstr_t* fs_make_fname(const char* dir_name, const char* name) {
	tstr_t* fullstr = tstr_new(dir_name, MFS);
	if(strcmp(dir_name, "/") != 0)
		tstr_addc(fullstr, '/');
	tstr_add(fullstr, name);
	return fullstr;
}
