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

int get_global(const char* key, proto_t* ret) {
	PF->clear(ret);

	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->adds(&in, key);
	int res = ipc_call(CORED_PID, CORE_CMD_GLOBAL_GET, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		res = proto_read_int(&out);
		if(res == 0) {
			res = proto_read_proto(&out, ret);
		}
	}
	return res;
}

int set_global_str(const char* key, const char* s) {
	proto_t in;
	PF->init(&in, NULL, 0)->adds(&in, s);
	int ret = set_global(key, &in);
	PF->clear(&in);
	return ret;
}

const char* get_global_str(const char* key) {
	static char s[256];	
	s[0] = 0;

	proto_t out;
	PF->init(&out, NULL, 0);
	int ret = get_global(key, &out);
	if(ret == 0) 
		strncpy(s, proto_read_str(&out), 255);	
	PF->clear(&out);
	return s;
}

