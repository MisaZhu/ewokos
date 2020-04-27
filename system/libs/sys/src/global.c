#include <sys/global.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/core.h>
#include <stddef.h>

int set_global(const char* key, proto_t* val) {
	proto_t in;
	proto_init(&in, NULL, 0);
	proto_add_str(&in, key);
	if(val != NULL)
		proto_add(&in, val->data, val->size);

	ipc_call(CORED_PID, CORE_CMD_GLOBAL_SET, &in, NULL);
	proto_clear(&in);
	return 0;
}

proto_t* get_global(const char* key) {
	proto_t in;
	proto_t* out;
	proto_init(&in, NULL, 0);
	out = proto_new(NULL, 0);
	proto_add_str(&in, key);
	ipc_call(CORED_PID, CORE_CMD_GLOBAL_GET, &in, out);
	proto_clear(&in);
	return out;
}
