#include <kserv.h>
#include <unistd.h>
#include <ipc.h>
#include <proto.h>
#include <vfs/vfs.h>
#include <vfs/vfscmd.h>
#include <fstree.h>
#include <kstring.h>
#include <stdlib.h>
#include <syscall.h>
#include <trunk.h>

#define PIPE_BUF_SIZE 4096
typedef struct {
	uint8_t buffer[PIPE_BUF_SIZE];
	uint32_t size;
	uint32_t offset;
}pipe_buffer_t;

int32_t do_pipe_open(int32_t pid, proto_t* out) {
	fs_info_t info;
	tree_node_t* node = fs_new_node();
	pipe_buffer_t* buffer = (pipe_buffer_t*)malloc(sizeof(pipe_buffer_t));
	memset(buffer, 0, sizeof(pipe_buffer_t));
	FSN(node)->data = buffer;
	info.size = 0;
	info.type = FS_TYPE_FILE;
	info.id = node->id;
	info.node = (uint32_t)node;
	info.dev_index = 0;
	info.data = NULL;
	info.dev_serv_pid = getpid();

	int32_t fd0 = syscall3(SYSCALL_PFILE_OPEN, pid, (int32_t)&info, O_RDONLY);
	if(fd0 < 0) {
		free(node);
		free(buffer);
		return -1;
	}

	int32_t fd1 = syscall3(SYSCALL_PFILE_OPEN, pid, (int32_t)&info, O_WRONLY);
	if(fd1 < 0) {
		free(node);
		free(buffer);
		return -1;
	}

	proto_add_int(out, fd0);
	proto_add_int(out, fd1);
	return 0;
}

void do_pipe_close(uint32_t node_addr, bool force) {
	if(!force && syscall2(SYSCALL_PFILE_GET_REF, (int32_t)node_addr, 2) > 0)
		return;
	tree_node_t* node = (tree_node_t*)node_addr;
	if(FSN(node)->data != NULL) {
		free(FSN(node)->data);
	}
	free(node);
}

int32_t do_pipe_write(int32_t pid, proto_t* in, proto_t* out) {
	int32_t fd = (uint32_t)proto_read_int(in);
	uint32_t size;
	void* p = proto_read(in, &size);
	int32_t seek = proto_read_int(in);
	(void)seek;
	
	errno = ENONE;
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0) {
		return -1;
	}

	int32_t ret = -1;
	int32_t ref = syscall2(SYSCALL_PFILE_GET_REF, info.node, 2);
	if(ref < 2) {//closed by other side of pipe.
		return -1;
	}

	tree_node_t* node = (tree_node_t*)info.node;
	pipe_buffer_t* buffer = (pipe_buffer_t*)FSN(node)->data;
	if(buffer == NULL) {
		return -1;
	}
	if(buffer->size == 0) {//ready for write.
		size = size < PIPE_BUF_SIZE ? size : PIPE_BUF_SIZE;
		memcpy(buffer->buffer, p, size);
		buffer->size = size;
		buffer->offset = 0;
		ret = size;
	}
	else {
		errno = EAGAIN;
		return -1;
	}
	proto_add_int(out, ret);
	return 0;
}

int32_t do_pipe_read(int32_t pid, proto_t* in, proto_t* out) {
	int32_t fd = (uint32_t)proto_read_int(in);
	uint32_t size = (uint32_t)proto_read_int(in);
	uint32_t seek = (uint32_t)proto_read_int(in);
	(void)seek;

	errno = ENONE;
	
	fs_info_t info;
	if(syscall3(SYSCALL_PFILE_INFO_BY_PID_FD, pid, fd, (int32_t)&info) != 0) {
		return -1;
	}
	if(size == 0) {
		proto_add_int(out, size);	
		return 0;
	}

	int32_t ret = 0;
	tree_node_t* node = (tree_node_t*)info.node;
	pipe_buffer_t* buffer = (pipe_buffer_t*)FSN(node)->data;
	if(buffer == NULL) {
		return -1;
	}
	uint32_t rest = buffer->size - buffer->offset;
	if(rest > 0 && rest <= PIPE_BUF_SIZE) {//ready for read.
		char* buf = NULL;
		size = size < rest ? size : rest;
		buf = (char*)malloc(size);
		memcpy(buf, buffer->buffer+buffer->offset, size);
		ret = size;
		buffer->offset += size;
		if(buffer->offset == buffer->size) {
			buffer->offset = 0;
			buffer->size = 0;
		}
		proto_add(out, buf, ret);
		free(buf);
		return 0;
	}
	else {
		int32_t ref = syscall2(SYSCALL_PFILE_GET_REF, info.node, 2);
		if(ref < 2) //closed by other side of pipe.
			return -1;
		else {
			errno = EAGAIN;
			return -1;
		}
	}
}
