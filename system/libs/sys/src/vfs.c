#include <sys/vfs.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <mstr.h>
#include <sys/ipc.h>

int vfs_new_node(fsinfo_t* info) {
	return syscall1(SYS_VFS_NEW_NODE, (int32_t)info);
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

int vfs_get(const char* fname, fsinfo_t* info) {
	fname = vfs_fullname(fname);
	int res = syscall2(SYS_VFS_GET, (int32_t)fname, (int32_t)info);
	return res;
}

int  vfs_access(const char* fname) {
	fsinfo_t info;
	return vfs_get(fname, &info);
}

int vfs_first_kid(fsinfo_t* info, fsinfo_t* ret) {
	return syscall2(SYS_VFS_FKID, (int32_t)info, (int32_t)ret);
}

int vfs_next(fsinfo_t* info, fsinfo_t* ret) {
	return syscall2(SYS_VFS_NEXT, (int32_t)info, (int32_t)ret);
}

int vfs_father(fsinfo_t* info, fsinfo_t* ret) {
	return syscall2(SYS_VFS_FATHER, (int32_t)info, (int32_t)ret);
}

int vfs_set(fsinfo_t* info) {
	return syscall1(SYS_VFS_SET, (int32_t)info);
}

int vfs_add(fsinfo_t* to, fsinfo_t* info) {
	return syscall2(SYS_VFS_ADD, (int32_t)to, (int32_t)info);
}

int vfs_del(fsinfo_t* info) {
	return syscall1(SYS_VFS_DEL, (int32_t)info);
}

int vfs_get_mount(fsinfo_t* info, mount_t* mount) {
	return syscall2(SYS_VFS_GET_MOUNT, (int32_t)info, (int32_t)mount);
}

int vfs_mount(fsinfo_t* mount_to, fsinfo_t* info) {
	return syscall2(SYS_VFS_MOUNT, (int32_t)mount_to, (int32_t)info);
}

int vfs_uumount(fsinfo_t* info) {
	return syscall1(SYS_VFS_UMOUNT, (int32_t)info);
}

int vfs_get_by_fd(int fd, fsinfo_t* info) {
	return syscall2(SYS_VFS_PROC_GET_BY_FD, (int32_t)fd, (int32_t)info);
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
	
	mount_t mount;
	if(vfs_get_mount(&info_to, &mount) != 0) {
		str_free(dir);
		str_free(name);
		return -1;
	}

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
	if(type == FS_TYPE_DEV)
		return 0;
	
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, FS_CMD_CREATE);
	proto_add(&in, &info_to, sizeof(fsinfo_t));
	proto_add(&in, ret, sizeof(fsinfo_t));

	int res = -1;
	if(ipc_call(mount.pid, &in, &out) != 0) {
		vfs_del(ret);
	}
	else {
		res = proto_read_int(&out);
		if(res == 0) {
			proto_read_to(&out, ret, sizeof(fsinfo_t));
			res = vfs_set(ret);
		}
	}
	proto_clear(&in);
	proto_clear(&out);
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

