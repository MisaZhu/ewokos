#include <sys/global.h>
#include <sys/syscall.h>
#include <stddef.h>

int set_global(const char* key, proto_t* in) {
	(void)key;
	(void)in;
	return 0;
}

proto_t* get_global(const char* key) {
	(void)key;
	return NULL;
}
