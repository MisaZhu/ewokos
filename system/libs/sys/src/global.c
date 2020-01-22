#include <sys/global.h>
#include <sys/syscall.h>

int set_global(const char* name, const char* value) {
	return syscall2(SYS_SET_GLOBAL, (int32_t)name, (int32_t)value);
}

static int get_global_raw(const char* name, char* value, int size) {
	return syscall3(SYS_GET_GLOBAL, (int32_t)name, (int32_t)value, size);
}

const char* get_global(const char* name) {
	static char v[1024];	
	if(get_global_raw(name, v, 1023) == 0)
		return v;
	return  "";
}
