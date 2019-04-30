#include <kserv.h>
#include <vfs/fs.h>
#include <vfs/vfs.h>
#include <ipc.h>
#include <kstring.h>
#include <syscall.h>
#include <stdlib.h>
#include <proto.h>
#include <unistd.h>

#define FS_BUF_SIZE (16*KB)

int fs_open(const char* name, int32_t flags) {
	fs_info_t info;
	if(vfs_node_by_name(name, &info) != 0)
		return -1;
	
	int32_t fd = syscall2(SYSCALL_PFILE_OPEN, (int32_t)&info, flags);
	if(fd < 0)
		return -1;

	if(info.dev_serv_pid == 0) 
		return fd;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_int(proto, flags);

	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_OPEN, proto->data, proto->size, true);
	proto_free(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return fd;
}

int fs_close(int fd) {
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	if(info.dev_serv_pid > 0) {
		ipc_req(info.dev_serv_pid, 0, FS_CLOSE, &info.node, 4, false);
		/*
		package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_CLOSE, (void*)&node, 4, false;);
		if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
			if(pkg != NULL) free(pkg);
			return -1;
		}
		free(pkg);
		*/
	}
	return syscall1(SYSCALL_PFILE_CLOSE, fd);
}

int32_t fs_dma(int fd, uint32_t* size) {
	*size = 0;
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	if(info.dev_serv_pid == 0) 
		return -1;
	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_DMA, &info.node, 4, true);
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
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	
	if(info.dev_serv_pid > 0) {
		ipc_req(info.dev_serv_pid, 0, FS_FLUSH, &info.node, 4, false);
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

int fs_read(int fd, char* buf, uint32_t size) {
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	uint32_t buf_size = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;
	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_int(proto, size);
	proto_add_int(proto, seek);
	package_t* pkg = ipc_req(info.dev_serv_pid, buf_size, FS_READ, proto->data, proto->size, true);
	proto_free(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
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

int fs_ctrl(int fd, int32_t cmd, void* input, uint32_t isize, void* output, uint32_t osize) {
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_int(proto, cmd);
	proto_add(proto, input, isize);
	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_CTRL, proto->data, proto->size, true);
	proto_free(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
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
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	uint32_t buf_size = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;

	int seek = syscall1(SYSCALL_PFILE_GET_SEEK, fd);
	if(seek < 0)
		return -1;

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add(proto, (void*)buf, size);
	proto_add_int(proto, seek);
	package_t* pkg = ipc_req(info.dev_serv_pid, buf_size, FS_WRITE, proto->data, proto->size, true);
	proto_free(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = *(int*)get_pkg_data(pkg);
	free(pkg);

	seek += sz;
	syscall2(SYSCALL_PFILE_SEEK, fd, seek);
	return sz;
}

int fs_add(int fd, const char* name) {
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return -1;
	
	int size = strlen(name);
	if(size == 0)
		return -1;

	if(info.dev_serv_pid == 0) {
		return vfs_add(info.node, name, 0, NULL);
	}

	proto_t* proto = proto_new(NULL, 0);
	proto_add_int(proto, info.node);
	proto_add_str(proto, name);
	package_t* pkg = ipc_req(info.dev_serv_pid, 0, FS_ADD, proto->data, proto->size, true);
	proto_free(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = *(int*)get_pkg_data(pkg);
	free(pkg);
	return sz;
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
	return vfs_node_by_name(name, info);
}

int fs_info(int fd, fs_info_t* info) {
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)info) != 0)
		return -1;
	return 0;
}

int fs_ninfo(uint32_t node_addr, fs_info_t* info) {
	if(syscall2(SYSCALL_PFILE_NODE_BY_ADDR, node_addr, (int32_t)info) != 0)
		return -1;
	return 0;
}

fs_info_t* fs_kids(int fd, uint32_t *num) {
	fs_info_t info;
	if(syscall2(SYSCALL_PFILE_NODE_BY_FD, fd, (int32_t)&info) != 0)
		return NULL;
	return vfs_kids(info.node, num);
}

int32_t fs_inited() {
	return kserv_get_pid("dev.sdcard");
}
