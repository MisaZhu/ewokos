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

#define PIPE_BUF_SIZE 4096
typedef struct {
	uint8_t buffer[PIPE_BUF_SIZE];
	uint32_t size;
	uint32_t offset;
}pipe_buffer_t;

void do_pipe_open(package_t* pkg) {
	fs_info_t info;
	tree_node_t* node = fs_new_node();
	pipe_buffer_t* buffer = (pipe_buffer_t*)malloc(sizeof(pipe_buffer_t));
	memset(buffer, 0, sizeof(pipe_buffer_t));
	FSN(node)->data = buffer;
	info.size = 0;
	info.type = FS_TYPE_FILE;
	info.id = node->id;
	info.node = (uint32_t)node;
	info.name[0] = 0;
	info.dev_index = 0;
	info.data = NULL;
	info.dev_serv_pid = getpid();
	ipc_send(pkg->id, pkg->type, &info, sizeof(fs_info_t));
}

void do_pipe_close(package_t* pkg) {
	fs_info_t* info = (fs_info_t*)get_pkg_data(pkg);
	if(info == NULL || info->node == 0)
		return;
	if(syscall2(SYSCALL_PFILE_GET_REF, info->node, 2) > 0) //1 ref left for this closing fd
		return;
	
	tree_node_t* node = (tree_node_t*)info->node;
	if(FSN(node)->data != NULL) {
		free(FSN(node)->data);
	}
	free(node);
}

void do_pipe_write(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node_addr = (uint32_t)proto_read_int(proto);
	uint32_t size;
	void* p = proto_read(proto, &size);
	int32_t seek = proto_read_int(proto);
	(void)seek;
	proto_free(proto);

	if(node_addr == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = -1;
	int32_t ref = syscall2(SYSCALL_PFILE_GET_REF, node_addr, 2);
	if(ref < 2) {//closed by other side of pipe.
		ipc_send(pkg->id, pkg->type, &ret, 4);
		return;
	}

	tree_node_t* node = (tree_node_t*)node_addr;
	pipe_buffer_t* buffer = (pipe_buffer_t*)FSN(node)->data;
	if(buffer == NULL) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	if(buffer->size == 0) {//ready for write.
		size = size < PIPE_BUF_SIZE ? size : PIPE_BUF_SIZE;
		memcpy(buffer->buffer, p, size);
		buffer->size = size;
		buffer->offset = 0;
		ret = size;
	}
	else {
		ipc_send(pkg->id, PKG_TYPE_AGAIN, NULL, 0);
		return;
	}
	ipc_send(pkg->id, pkg->type, &ret, 4);
}

void do_pipe_read(package_t* pkg) {
	proto_t* proto = proto_new(get_pkg_data(pkg), pkg->size);
	uint32_t node_addr = (uint32_t)proto_read_int(proto);
	uint32_t size = (uint32_t)proto_read_int(proto);
	uint32_t seek = (uint32_t)proto_read_int(proto);
	(void)seek;
	proto_free(proto);
	
	if(node_addr == 0) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}
	if(size == 0) {
		ipc_send(pkg->id, pkg->type, NULL, 0);
		return;
	}

	int32_t ret = 0;
	tree_node_t* node = (tree_node_t*)node_addr;
	pipe_buffer_t* buffer = (pipe_buffer_t*)FSN(node)->data;
	if(buffer == NULL) {
		ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
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
		ipc_send(pkg->id, pkg->type, buf, ret);
		free(buf);
	}
	else {
		int32_t ref = syscall2(SYSCALL_PFILE_GET_REF, node_addr, 2);
		if(ref < 2) //closed by other side of pipe.
			ipc_send(pkg->id, PKG_TYPE_ERR, NULL, 0);
		else 
			ipc_send(pkg->id, PKG_TYPE_AGAIN, NULL, 0);
	}
}

