#include "mario.h"

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*=====system native functions=========*/
var_t* native_system_sleep(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	uint32_t sec = (uint32_t)get_int(env, "sec");
	sleep(sec);
	return NULL;
}

var_t* native_system_usleep(vm_t* vm, var_t* env, void* data) {
	(void)vm; (void)data;
	uint32_t usec = (uint32_t)get_int(env, "usec");
	usleep(usec);
	return NULL;
}

#define CLS_SYSTEM "System"
void reg_native_system(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_SYSTEM);
	vm_reg_static(vm, cls, "sleep(sec)", native_system_sleep, NULL);
	vm_reg_static(vm, cls, "usleep(usec)", native_system_usleep, NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

