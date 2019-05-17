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

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, fd);
	proto_add_int(proto, flags);

	package_t* pkg = ipc_req(dev_serv_pid, 0, FS_OPEN, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
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
	*size = 0;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid <= 0)
		return -1;
	package_t* pkg = ipc_req(dev_serv_pid, 0, FS_DMA, &fd, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	int32_t ret = proto_read_int(proto);
	*size = proto_read_int(proto);
	proto_free(proto);
	free(pkg);
	return ret;
}

int32_t fs_flush(int fd) {
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;
	
	if(dev_serv_pid > 0) {
		ipc_req(dev_serv_pid, 0, FS_FLUSH, &fd, 4, false);
		/*
		package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_FLUSH, (void*)&node, 4, true);
		if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
			if(pkg != NULL) free(pkg);
			return -1;
		}
		free(pkg);
		*/
	}
	return 0;
}

/*
int fs_info(int fd, fs_info_t* info) {
	if(fd < 0 || syscall2(SYSCALL_PFILE_INFO_BY_FD, fd, (int32_t)info) != 0)
		return -1;
	return 0;
}
*/

int fs_read(int fd, char* buf, uint32_t size) {
	if(fd < 0)
		return -1;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;
	uint32_t buf_size = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;
	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, fd);
	proto_add_int(proto, size);
	proto_add_int(proto, seek);
	package_t* pkg = ipc_req(dev_serv_pid, buf_size, FS_READ, proto->data, proto->size, true);
	proto_free(proto);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg->type == PKG_TYPE_AGAIN)
			errno = EAGAIN;
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = pkg->size;
	if(sz == 0) {
		free(pkg);
		return 0;
	}
	
	memcpy(buf, get_pkg_data(pkg), sz);
	free(pkg);

	seek += sz;
	syscall2(SYSCALL_PFILE_SEEK, fd, seek);
	return sz;
}

int fs_fctrl(const char* fname, int32_t cmd, void* input, uint32_t isize, void* output, uint32_t osize) {
	if(fname[0] == 0)
		return -1;
	fs_info_t info;
	if(fs_finfo(fname, &info) != 0)
		return -1;
	if(info.dev_serv_pid < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_str(proto, fname);
	proto_add_int(proto, cmd);
	proto_add(proto, input, isize);
	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_FCTRL, proto->data, proto->size, true);
	proto_free(proto);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg->type == PKG_TYPE_AGAIN)
			errno = EAGAIN;
		return -1;
	}

	if(output == NULL || osize == 0) {
		free(pkg);
		return 0;
	}
	uint32_t sz = pkg->size;

	if(sz > osize)
		sz = osize;
	memcpy(output, get_pkg_data(pkg), sz);
	free(pkg);
	return 0;
}


int fs_ctrl(int fd, int32_t cmd, void* input, uint32_t isize, void* output, uint32_t osize) {
	if(fd < 0)
		return -1;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, fd);
	proto_add_int(proto, cmd);
	proto_add(proto, input, isize);
	package_t* pkg = ipc_req(dev_serv_pid, 0, FS_CTRL, proto->data, proto->size, true);
	proto_free(proto);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg->type == PKG_TYPE_AGAIN)
			errno = EAGAIN;
		return -1;
	}

	if(output == NULL || osize == 0) {
		free(pkg);
		return 0;
	}
	uint32_t sz = pkg->size;

	if(sz > osize)
		sz = osize;
	memcpy(output, get_pkg_data(pkg), sz);
	free(pkg);
	return 0;
}

int fs_write(int fd, const char* buf, uint32_t size) {
	if(fd < 0)
		return -1;
	int32_t dev_serv_pid = syscall1(SYSCALL_PFILE_SERV_PID_BY_FD, fd);
	if(dev_serv_pid < 0)
		return -1;
	uint32_t buf_size = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;

	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, fd);
	proto_add(proto, (void*)buf, size);
	proto_add_int(proto, seek);
	package_t* pkg = ipc_req(dev_serv_pid, buf_size, FS_WRITE, proto->data, proto->size, true);
	proto_free(proto);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg->type == PKG_TYPE_AGAIN)
			errno = EAGAIN;
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = *(int*)get_pkg_data(pkg);
	free(pkg);

	seek += sz;
	syscall2(SYSCALL_PFILE_SEEK, fd, seek);
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
		package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_ADD, proto->data, proto->size, true);
		proto_free(proto);
		if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
			if(pkg != NULL) free(pkg);
			vfs_del(CS(fname));
			ret = -1;
		}
		else {
			ret = 0;
		}
		tstr_free(fname);
		free(pkg);
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
