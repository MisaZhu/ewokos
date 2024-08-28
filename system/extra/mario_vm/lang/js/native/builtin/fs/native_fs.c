#include "native_fs.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*=====fs native functions=========*/
var_t* native_fs_close(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	int fd = get_int(env, "fd");
	close(fd);
	return NULL;
}

var_t* native_fs_open(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;

	const char* fname = get_str(env, "fname");
	int flags = get_int(env, "flags");

	int res = open(fname, flags);
	return var_new_int(vm, res);
}

var_t* native_fs_write(vm_t* vm, var_t* env, void* data) {
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

var_t* native_fs_read(vm_t* vm, var_t* env, void* data) {
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

#define CLS_FS "FS"

void reg_native_fs(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_FS);
	vm_reg_var(vm, cls, "RDONLY", var_new_int(vm, O_RDONLY), true);
	vm_reg_var(vm, cls, "WRONLY", var_new_int(vm, O_WRONLY), true);
	vm_reg_var(vm, cls, "RDWR", var_new_int(vm, O_RDWR), true);

	vm_reg_static(vm, cls, "close(fd)", native_fs_close, NULL);
	vm_reg_static(vm, cls, "open(fname, flags)", native_fs_open, NULL);
	vm_reg_static(vm, cls, "write(fd, bytes, size)", native_fs_write, NULL);
	vm_reg_static(vm, cls, "read(fd, bytes, size)", native_fs_read, NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

