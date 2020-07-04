#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/proto.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/mstr.h>
#include <sys/shm.h>
#include <errno.h>
//#include <rawdata.h>

int errno = ENONE;

int getuid(void) {
	return syscall0(SYS_PROC_GET_UID);
}

int setuid(int uid) {
	return syscall1(SYS_PROC_SET_UID, uid);
}

int getpid_raw(int pid) {
	return syscall1(SYS_GET_PID, pid);
}

int getpid(void) {
	return getpid_raw(-1);
}

int fork(void) {
	return syscall0(SYS_FORK);
}

int usleep(unsigned int usecs) {
	if(usecs == 0)
		syscall0(SYS_YIELD);
	else
		syscall1(SYS_USLEEP, usecs);
	return 0;
}

unsigned int sleep(unsigned int seconds) {
	usleep(seconds * 1000 * 1000);
	return 0;
}

int read(int fd, void* buf, uint32_t size) {
	errno = ENONE;
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int res = -1;
	if(info.type == FS_TYPE_PIPE) {
		while(1) {
			res = vfs_read_pipe(&info, buf, size, 1);
			if(res >= 0)
				break;
			if(errno != EAGAIN)
				break;
			sleep(0);
		}
		return res;
	}

	while(1) {
		res = vfs_read(fd, &info, buf, size);
		if(res >= 0)
			break;
		if(errno != EAGAIN)
			break;
		if((info.type & FS_TYPE_SYNC) != 0)
			proc_block(info.mount_pid, 0);
		else
			sleep(0);
	}
	return res;
}


int write(int fd, const void* buf, uint32_t size) {
	errno = ENONE;
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int res = -1;
	if(info.type == FS_TYPE_PIPE) {
		while(1) {
			res = vfs_write_pipe(&info, buf, size, 1);
			if(res >= 0)
				break;
			if(errno != EAGAIN)
				break;
			sleep(0);
		}
		return res;
	}

	while(1) {
		res = vfs_write(fd, &info, buf, size);
		if(res >= 0)
			break;
		if(errno != EAGAIN)
			break;
		sleep(0);
	}
	return res;
}

int lseek(int fd, uint32_t offset, int whence) {
	if(whence == SEEK_CUR) {
		int cur = vfs_tell(fd);
		if(cur < 0)
			cur = 0;
		offset += cur;
	}	
	else if(whence == SEEK_END) {
		fsinfo_t info;
		int cur = 0;
		if(vfs_get_by_fd(fd, &info) == 0)
			cur = info.size;
		offset += cur;
	}
	return vfs_seek(fd, offset);
}

void exec_elf(const char* cmd_line, const char* elf, int32_t size) {
	syscall3(SYS_EXEC_ELF, (int32_t)cmd_line, (int32_t)elf, size);
}

int exec(const char* cmd_line) {
	str_t* cmd = str_new("");
	const char *p = cmd_line;
	while(*p != 0 && *p != ' ') {
		str_addc(cmd, *p);
		p++;
	}
	str_addc(cmd, 0);
	int sz;
	void* buf = vfs_readfile(cmd->cstr, &sz);
	if(buf == NULL) {
		str_free(cmd);
		return -1;
	}
	exec_elf(cmd_line, buf, sz);
	free(buf);
	return 0;
}

inline static int get_procd_pid(void) {
	return ipc_serv_get(IPC_SERV_PROC);
}

char* getcwd(char* buf, uint32_t size) {
	proto_t out;
	PF->init(&out, NULL, 0);
	if(ipc_call(get_procd_pid(), PROC_CMD_GET_CWD, NULL, &out) == 0) {
		if(proto_read_int(&out) == 0) {
			strncpy(buf, proto_read_str(&out), size-1);
		}
	}
	PF->clear(&out);
	return buf;
}

int chdir(const char* path) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->adds(&in, path);
	int res = ipc_call(get_procd_pid(), PROC_CMD_SET_CWD, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&out);
	return res;
}

int dup2(int from, int to) {
	return vfs_dup2(from, to);
}

int dup(int from) {
	return vfs_dup(from);
}

int pipe(int fds[2]) {
	return vfs_open_pipe(fds);
}

int unlink(const char* fname) {
	fsinfo_t info;
	if(vfs_get(fname, &info) != 0)
		return -1;
	if(info.type != FS_TYPE_FILE) 
		return -1;
	
	/*mount_t mount;
	if(vfs_get_mount(&info, &mount) != 0)
		return -1;
		*/
	
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		add(&in, &info, sizeof(fsinfo_t))->
		adds(&in, fname);

	ipc_call(info.mount_pid, FS_CMD_UNLINK, &in, &out);
	PF->clear(&in);
	int res = proto_read_int(&out);
	PF->clear(&out);
	if(res == 0)
		return vfs_del(&info);
	return -1;
}
