#include "native_dir.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


static void destroyDir(void* data) {
	DIR* dir = (DIR*)data;
	if(dir != NULL)
		closedir(dir);
}

var_t* native_dir_close(vm_t* vm, var_t* env, void* data) {
	var_t* thisV = get_obj(env, THIS);
	var_clean(thisV);
	return NULL;
}

var_t* native_dir_read(vm_t* vm, var_t* env, void* data) {
	DIR* dir = (DIR*)get_raw(env, THIS);
	if(dir == NULL)
		return var_new_str(vm, "");

	struct dirent* dp = readdir(dir);
	if(dp == NULL)
		return var_new_str(vm, "");
	
	return var_new_str(vm, dp->d_name);
}


var_t* native_dir_open(vm_t* vm, var_t* env, void* data) {
	const char*  name = get_str(env, "name");
	
	if(name[0] == 0)
		return var_new_int(vm, 0);

	DIR* d = opendir(name);
	if(d == NULL)
		return NULL;
	
	var_t* thisV = var_new_obj(vm, d, destroyDir);
	var_instance_from(thisV, get_obj(env, THIS));
	return thisV;
}

#define CLS_DIR "Dir"

void reg_native_dir(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_DIR);
	vm_reg_native(vm, cls, "close()", native_dir_close, NULL);
	vm_reg_static(vm, cls, "open(name)", native_dir_open, NULL);
	vm_reg_native(vm, cls, "constructor(name)", native_dir_open, NULL);
	vm_reg_native(vm, cls, "read()", native_dir_read, NULL);
}	
