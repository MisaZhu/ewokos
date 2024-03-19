#include "native_socket.h"

#include <unistd.h>
//#include <sys/ioctl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*=====socket native functions=========*/
var_t* native_socket_socket(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int type = get_int(env, "type");
	int fd = -1;
	if(type == 0) { //tcp
		fd = socket(PF_INET, SOCK_STREAM, 0);
	}
	else {
		fd = socket(PF_INET, SOCK_DGRAM, 0);
	}

	return var_new_int(vm, fd);
}

var_t* native_socket_close(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");

	close(fd);
	return NULL;
}

var_t* native_socket_shutdown(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");
	shutdown(fd, SHUT_RDWR);
	return NULL;
}

var_t* native_socket_connect(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");
	const char* host = get_str(env, "host");
	int port = get_int(env, "port");
	int timeout = get_int(env, "timeout");

	int res = -1;
	if(fd < 0 || port < 0)
		return var_new_int(vm, res);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port=htons(port);

	if(timeout <= 0) {
		res = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	}
	else {
		unsigned long ul = 1;
		//ioctl(fd, FIONBIO, &ul); //TODO

		if(connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) {
			int error=-1, len;

			struct timeval tm;
			tm.tv_sec = timeout;
			tm.tv_usec = 0;

			fd_set set;
			FD_ZERO(&set);
			FD_SET(fd, &set);

			if( select(fd+1, NULL, &set, NULL, &tm) > 0) {
				getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
				if(error == 0) {
					res = 0;
				}
			}
		}

		ul = 0;
		//ioctl(fd, FIONBIO, &ul); //TODO
	}

	return var_new_int(vm, res);
}

var_t* native_socket_bind(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");
	const char* host = get_str(env, "host");
	int port = get_int(env, "port");

	int res = -1;
	if(fd < 0 || port <= 0) 
		return var_new_int(vm, res);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port=htons(port);
	if(host[0] != 0)
		addr.sin_addr.s_addr = inet_addr(host);

	res = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	return var_new_int(vm, res);
}

var_t* native_socket_accept(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");
	struct sockaddr_in in;
	memset(&in, 0, sizeof(in));
	in.sin_family = AF_INET;
	socklen_t size = sizeof(struct sockaddr);

	int cid = accept(fd, (struct sockaddr *)&in, &size);
	return var_new_int(vm, cid);
}

var_t* native_socket_listen(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");
	int backlog = get_int(env, "backlog");

	int res = listen(fd, backlog);
	return var_new_int(vm, res);
}

var_t* native_socket_write(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");

	node_t* n = var_find(env, "bytes");
	if(n == NULL || n->var == NULL || n->var->size == 0)
		return NULL;
	var_t* bytes = n->var;

	int bytesSize = bytes->size;
	if(bytes->type == V_STRING)
		bytesSize--;

	int size = get_int(env, "size");
	if(size == 0 || size > bytesSize)
		size = bytesSize;

	int res = write(fd, (const char*)bytes->value, size);
	return var_new_int(vm, res);
}

var_t* native_socket_read(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");

	node_t* n = var_find(env, "bytes");
	if(n == NULL || n->var == NULL || n->var->size == 0)
		return NULL;
	var_t* bytes = n->var;

	int bytesSize = bytes->size;

	int size = get_int(env, "size");
	if(size == 0 || size > bytesSize)
		size = bytesSize;

	char* s = (char*)bytes->value;
	int res = read(fd, s, size);
	if(res >= 0)
		s[res] = 0;
	return var_new_int(vm, res);
}

var_t* native_socket_setTimeout(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");
	int timeout = get_int(env, "timeout");

	struct timeval tv_timeout;
	tv_timeout.tv_sec = timeout;
	tv_timeout.tv_usec = 0;

	int res = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void *) &tv_timeout, sizeof(struct timeval));
	return var_new_int(vm, res);
}

#define CLS_SOCKET "Socket"

void reg_native_socket(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_SOCKET);
	vm_reg_var(vm, cls, "TCP", var_new_int(vm, 0), true);
	vm_reg_var(vm, cls, "UDP", var_new_int(vm, 1), true);

	vm_reg_static(vm, cls, "socket(type)", native_socket_socket, NULL);
	vm_reg_static(vm, cls, "close(fd)", native_socket_close, NULL);
	vm_reg_static(vm, cls, "shutdown(fd)", native_socket_shutdown, NULL);
	vm_reg_static(vm, cls, "connect(fd, host, port, timeout)", native_socket_connect, NULL);
	vm_reg_static(vm, cls, "bind(fd, host, port)", native_socket_bind, NULL);
	vm_reg_static(vm, cls, "accept(fd)", native_socket_accept, NULL);
	vm_reg_static(vm, cls, "listen(fd, backlog)", native_socket_listen, NULL);
	vm_reg_static(vm, cls, "write(fd, bytes, size)", native_socket_write, NULL);
	vm_reg_static(vm, cls, "read(fd, bytes, size)", native_socket_read, NULL);
	vm_reg_static(vm, cls, "setTimeout(fd, timeout)", native_socket_setTimeout, NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

