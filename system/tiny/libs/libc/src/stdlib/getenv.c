#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <string.h>

const char* getenv(const char* name) {
	static char ret[1024];
	ret[0] = 0;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->adds(&in, name);

	int res = ipc_call(get_procd_pid(), PROC_CMD_GET_ENV, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		if(proto_read_int(&out) == 0) {
			strncpy(ret, proto_read_str(&out), 1023);
		}
	}
	PF->clear(&out);
	return ret;
}

