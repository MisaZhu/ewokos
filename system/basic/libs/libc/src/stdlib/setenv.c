#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <string.h>

int setenv(const char* name, const char* value) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->adds(&in, name)->adds(&in, value);

	int res = ipc_call(get_procd_pid(), PROC_CMD_SET_ENV, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		if(proto_read_int(&out) != 0) {
			res = -1;
		}
	}
	else {
		res = -1;
	}
	PF->clear(&out);
	return res;
}

