#include <sys/global.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/core.h>
#include <stddef.h>

int set_global(const char* key, proto_t* val) {
	proto_t in;
	PF->init(&in, NULL, 0)->adds(&in, key);
	if(val != NULL)
		PF->add(&in, val->data, val->size);

	ipc_call(CORED_PID, CORE_CMD_GLOBAL_SET, &in, NULL);
	PF->clear(&in);
	return 0;
}

proto_t* get_global(const char* key) {
	proto_t in;
	proto_t* out;
	out = proto_new(NULL, 0);
	PF->init(&in, NULL, 0)->adds(&in, key);
	ipc_call(CORED_PID, CORE_CMD_GLOBAL_GET, &in, out);
	PF->clear(&in);
	return out;
}
